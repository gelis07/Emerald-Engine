#define GLM_FORCE_ALIGNED_GENTYPES
#include "Renderer.h"
#include <glm/glm.hpp>
#include <vector>
#include <fmt/base.h>

struct alignas(16) ModelInfoData {
    glm::mat4 matrixModel;
    glm::mat4 invMatrixModel;
    int matIndex;
    int padding[3];
};
struct MeshInfoData
{
    int modelIdx;
    int textureIdx;
};

void Renderer::Render(RenderSettings& rs)
{
    if(rs.spp <= 0)
    {
        fmt::println("proper samples per pixel needed");
        return;
    }

    for(int i = 0; i < rs.spp; i++)
    {
        RenderSample(rs);
        fmt::println("progress: {}%", (float(i) / float(rs.spp)) * 100.0f);
    }

    //Post processing
    glBindImageTexture(0, rs.imageOut, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);
    postProcessing.Bind();
    postProcessing.Uniform1i("spp", rs.spp);
    glDispatchCompute((unsigned int)rs.ImgWidth/16, (unsigned int)rs.ImgHeight/16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glFinish();
}

void Renderer::RenderSample(RenderSettings& rs)
{
    frames++;
    UpdateSettings(rs);

    glBindImageTexture(0, rs.imageOut, 0, GL_FALSE,0 ,GL_READ_WRITE, GL_RGBA32F);
    Raytracer.Bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, TrianglesSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, AABBInfo);

    Raytracer.Uniform1f("AR", AR);
    Raytracer.Uniform1f("tfov", tfov);
    Raytracer.Uniform3f("CamPos", rs.scene.camera.GetPos());

    Raytracer.Uniform1i("uframe", frames);
    Raytracer.UniformMat4("InvProj", rs.scene.camera.GetInvProjection());
    Raytracer.UniformMat4("InvView", rs.scene.camera.GetInvView());
    std::vector<ModelInfoData> modelInfo;
    std::vector<MeshInfoData> meshInfo;
    for(int i = 0; i < rs.scene.models.size(); i++)
    {
        ModelInfoData modelInfoData;
        modelInfoData.matrixModel = rs.scene.models[i].model;
        modelInfoData.invMatrixModel = rs.scene.models[i].GetModelInverse();
        modelInfoData.matIndex = rs.scene.models[i].matIndex;
        modelInfo.push_back(modelInfoData);
    }
    glNamedBufferSubData(ModelInfo,0,sizeof(ModelInfoData) * modelInfo.size(),modelInfo.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ModelInfo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, AABBIndices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, MeshInfo);
    Raytracer.Uniform1i(std::string("modelAABBCount"), mModelAabbIdcs.size());

    Raytracer.Uniform1i("TriCount", mSceneTriCount);
    for(int i = 0; i < TextureIds.size(); i++)
    {
        std::string indexString = std::to_string(i);
        glActiveTexture(GL_TEXTURE0+i+1);
        glBindTexture(GL_TEXTURE_2D, TextureIds[i]);
        Raytracer.Uniform1i("texImage[" + indexString + "]", i+1);
    }
    for(int i = 0; i < rs.scene.materials.size(); i++)
    {
        const Material& mat = (*rs.scene.materials[i]);
        std::string indexString = std::to_string(i);
        Raytracer.Uniform3f(std::string("materials[" + indexString + "].albedo"), mat.albedo);
        Raytracer.Uniform3f(std::string("materials[" + indexString + "].emmColor"), mat.emmColor);
        Raytracer.Uniform1i(std::string("materials[" + indexString + "].materialType"), mat.scatter);
        Raytracer.Uniform1f(std::string("materials[" + indexString + "].metallic"), mat.metallic);
        Raytracer.Uniform1f(std::string("materials[" + indexString + "].roughness"), mat.roughness);
    }


    glDispatchCompute((unsigned int)rs.ImgWidth/16, (unsigned int)rs.ImgHeight/16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glFinish();
}

void Renderer::UpdateSettings(RenderSettings& rs)
{
    if(prevModelCount != rs.scene.models.size() || rs.ReloadScene)
    {
        AABBSetupGPU(rs.scene);
        prevModelCount = rs.scene.models.size();
        rs.ReloadScene = false;
        mSceneTriCount = 0;
        for (int i = 0; i < rs.scene.models.size(); i++)
        {
            for(int j = 0; j < rs.scene.models[i].mMeshes.size(); j++)
            {
                mSceneTriCount += rs.scene.models[i].mMeshes[j].indices.size();
            }
        }
    }
}

void Renderer::Init(const RenderSettings& rs, int width, int heigth)
{
    std::vector<float> QuadVertices = 
    {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };
    //Initialize shaders
    Raytracer.Init();
    Raytracer.LinkShader("../Shaders/raytracer.comp", GL_COMPUTE_SHADER);
    postProcessing.Init();
    postProcessing.LinkShader("../Shaders/PostProcessing.comp", GL_COMPUTE_SHADER);

    tfov = glm::tan(3.14159 / 8);
    AR = (double)width / (double)heigth;
}


struct AABBGPUstruct
{
    glm::vec4 min;
    glm::vec4 max;
    int nodeA;
    int nodeB;
    int triIndex;
    int triCount;
};
struct GPUTriangle {
    glm::vec4 a;
    glm::vec4 b;
    glm::vec4 c;
    glm::vec4 idtex;
};
void Renderer::AABBSetupGPU(const Scene& scene)
{
    std::vector<GPUTriangle> triangles;
    std::vector<AABBGPUstruct> modelAABBs;
    int meshSize = 0;
    std::vector<MeshInfoData> meshInfo;
    for(int i = 0; i <scene.models.size(); i++)
    {
        unsigned int offset = meshSize;
        meshSize += scene.models[i].mMeshes.size();
        for(int j = 0; j < scene.models[i].mMeshes.size(); j++)   
        {
            const Mesh& mesh = scene.models[i].mMeshes[j];
            MeshInfoData data;
            data.modelIdx = i;
            if(mesh.texture.id != -1)
            {
                TextureIds.push_back(mesh.texture.id);
                data.textureIdx = TextureIds.size()-1;
            }
            else{
                data.textureIdx = -1;
            }
            meshInfo.push_back(data);
        }
    }
    int triCount = 0;
  
    mModelAabbIdcs.clear();
    mModelAabbIdcs.push_back(0);
    unsigned int offset = 0;
    for(int i = 0; i < scene.models.size(); i++)
    {
        const Model& model = scene.models[i];
        for (int j = 0; j < model.aabbs.size(); j++)
        {
            const AABB& aabb = model.aabbs[j];
            AABBGPUstruct aabbGPU;
            aabbGPU.min = glm::vec4(aabb.min, i);
            aabbGPU.max = glm::vec4(aabb.max, i);
            aabbGPU.nodeA = aabb.nodeA;
            aabbGPU.nodeB = aabb.nodeB;
            if(aabb.leaf)
            {
                aabbGPU.triIndex = triCount;
                for(int t = 0; t < aabb.mTriangleList.size(); t++)
                {
                    const Triangle& tri = aabb.mTriangleList[t];
                    GPUTriangle gpuTri;
                    gpuTri.a = glm::vec4(tri.a, tri.texA.x);
                    gpuTri.idtex.y = tri.texA.y;
                    gpuTri.b = glm::vec4(tri.b, tri.texB.x);
                    gpuTri.idtex.z = tri.texB.y;
                    gpuTri.c = glm::vec4(tri.c, tri.texC.x);
                    gpuTri.idtex.w = tri.texC.y;
                    gpuTri.idtex.x = offset + tri.meshIdx;
                    triangles.push_back(gpuTri);
                    triCount++;
                } 
                aabbGPU.triCount = aabb.mTriangleList.size();
            }
            else
            {
                aabbGPU.triCount = 0;
                aabbGPU.triIndex = 0;
            }   
            modelAABBs.push_back(aabbGPU);
        }
        offset += scene.models[i].mMeshes.size();
        if(i != scene.models.size() - 1)
            mModelAabbIdcs.push_back(modelAABBs.size());
    }

    if(!hasCreatedBuffers)
    {
        hasCreatedBuffers = true;
    }else{
        glDeleteBuffers(1, &AABBInfo);
        glDeleteBuffers(1, &TrianglesSSBO);
        glDeleteBuffers(1, &ModelInfo);
        glDeleteBuffers(1, &MeshInfo);
        glDeleteBuffers(1, &AABBIndices);
    }

    glCreateBuffers(1, &AABBInfo);
    glNamedBufferStorage(AABBInfo, sizeof(AABBGPUstruct) * modelAABBs.size(), (const void*) modelAABBs.data(), 0);
    glCreateBuffers(1, &TrianglesSSBO);
    glNamedBufferStorage(TrianglesSSBO, sizeof(GPUTriangle) * triangles.size(), (const void*) triangles.data(), 0);
    glCreateBuffers(1, &ModelInfo);
    glNamedBufferStorage(ModelInfo, sizeof(ModelInfoData) * scene.models.size(), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &MeshInfo);
    glNamedBufferStorage(MeshInfo, sizeof(MeshInfoData) * meshSize, (const void*)meshInfo.data(), 0);
    glCreateBuffers(1, &AABBIndices);
    glNamedBufferStorage(AABBIndices, sizeof(int) * mModelAabbIdcs.size(), mModelAabbIdcs.data(), 0);
}