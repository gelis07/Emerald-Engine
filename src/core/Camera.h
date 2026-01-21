#pragma once
#include <glm/glm.hpp>

class CameraControl;

struct CameraSettings
{
    glm::mat4 projection = glm::mat4(-1);
    glm::mat4 view = glm::mat4(-1);
    glm::vec3 dir = glm::vec3(INFINITY);
    glm::vec3 pos = glm::vec3(INFINITY);
    float fov = -1.0f;
    float nearClip = -1.0f;
    float farClip = -1.0f;   
};

class Camera
{
    public:

        void Set(const CameraSettings& stg)
        {
            if(stg.fov > 0) mFov = stg.fov;
            if(stg.nearClip > 0) mNearClip = stg.nearClip;
            if(stg.farClip> 0) mFarClip = stg.farClip;
            if(stg.projection != glm::mat4(-1))
            {
                mProjection = stg.projection;
                mInvProjection = glm::inverse(stg.projection);
            } 
            if(stg.view != glm::mat4(-1))
            {
                mView = stg.view;
                mInvView = glm::inverse(stg.view);
            }
            if(stg.dir != glm::vec3(INFINITY))
            {
                mDir = stg.dir;
            }
            if(stg.pos != glm::vec3(INFINITY))
            {
                mPos = stg.pos;
            }
        }
        CameraSettings fill()
        {
            CameraSettings stg;
            stg.dir = mDir;
            stg.farClip = mFarClip;
            stg.fov = mFov;
            stg.pos = mPos;
            stg.view = mView;
            stg.projection = mProjection;
            stg.nearClip = mNearClip;
            return stg;
        }


        const glm::mat4& GetProjection() const {return mProjection;} 
        const glm::mat4& GetInvProjection() const {return mInvProjection;}
        const glm::mat4& GetView() const {return mView;}
        const glm::mat4& GetInvView() const {return mInvView;} 
        const glm::vec3& GetPos() const {return mPos;}
        const glm::vec3& GetDirection() const {return mDir;}
        const float& GetFov() const {return mFov;}
        const float& GetNearClip() const {return mNearClip;}
        const float& GetFarClip() const {return mFarClip;}

        bool moved;
    private:
        glm::mat4 mProjection{1.0f};
        glm::mat4 mView{1.0f};
        glm::mat4 mInvProjection{1.0f};
        glm::mat4 mInvView{1.0f};
        glm::vec3 mPos{0.0f, 0.0f, 0.0f};
        glm::vec3 mDir{0.0f,0.0f, 0.0f};

        float mFov = 45.0f;
        float mNearClip = 0.1f;
        float mFarClip = 100.0f;
};