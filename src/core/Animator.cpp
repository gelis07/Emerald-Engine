#include "Animator.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

void ModifyNodeVar(LinearFunc func,float time, NodeData& node, Variable var, uint32_t varIdx)
{
    glm::vec3 scale, pos, skew;
    glm::vec4 pers;
    glm::quat rot;
    glm::decompose(node.transform, scale, rot, pos, skew, pers);
    glm::mat4 final(1.0f);
    switch(var)
    {
        case(POSITION):
        {
            glm::vec3 transform = pos;
            transform[varIdx] = func.a * time + func.b;
            final = glm::translate(final, transform);
            final = final * glm::mat4_cast(rot);
            final = glm::scale(final, scale);
            break;
        }
        case (ROTATION):
        {
            glm::vec3 transform = glm::eulerAngles(rot);
            transform[varIdx] = func.a * time + func.b;
            final = glm::translate(final, pos);
            final = glm::rotate(final, glm::radians(transform.x), glm::vec3(1, 0, 0));
            final = glm::rotate(final, glm::radians(transform.y), glm::vec3(0, 1, 0));
            final = glm::rotate(final, glm::radians(transform.z), glm::vec3(0, 0, 1));
            final = glm::scale(final, scale);
            break;
        }
        case (SCALE):
        {
            glm::vec3 transform = scale;
            transform[varIdx] = func.a * time + func.b;
            final = glm::translate(final, pos);
            final = final * glm::mat4_cast(rot);
            final = glm::scale(final, transform);
            break;
        }
    }

    node.transform = final;
}



void Animator::run(uint32_t frame, Scene& scene)
{
    float animTime = (float)animationFrame / timeStep;
    for (auto const& [key, val] : data)
    {
        NodeData& node = scene.models[key.modelId].nodeData[key.nodeId];

        for(int i = 0; i < val.intervals.size(); i++)
        {
            Keyframe fK = keyframes[val.intervals[i].fKeyframeIdx];
            Keyframe iK = keyframes[val.intervals[i].iKeyframeIdx];

            if(animTime >= iK.time && animTime <= fK.time)
                ModifyNodeVar(val.intervals[i].func, animTime, node, key.var, key.varIdx);
        }
    }
}


void Animator::findCorrectKeyframe(NodeVariableKey nodeKey)
{

    std::vector<uint32_t>& keyframeIdcs = data[nodeKey].keyframeIdcs;
    std::vector<Interval>& intervals = data[nodeKey].intervals;

    std::vector<Keyframe>& keyframesp = keyframes;
    std::sort(keyframeIdcs.begin(), keyframeIdcs.end(), [keyframesp](uint32_t a, uint32_t b)
    {
        return keyframesp[a].time < keyframesp[b].time;
    });
    intervals.clear();

    for(int i = 0; i < keyframeIdcs.size()-1; i++)
    {
        Interval interval;
        interval.iKeyframeIdx = keyframeIdcs[i];
        interval.fKeyframeIdx = keyframeIdcs[i+1];

        interval.func.a = (keyframes[interval.fKeyframeIdx].fvalue - keyframes[interval.iKeyframeIdx].fvalue)
        / (keyframes[interval.fKeyframeIdx].time - keyframes[interval.iKeyframeIdx].time);

        interval.func.b = keyframes[interval.iKeyframeIdx].fvalue - interval.func.a * keyframes[interval.iKeyframeIdx].time;
        intervals.push_back(interval);
    }
}

void Animator::addKeyframe(Keyframe key, NodeVariableKey nodeKey)
{
    std::vector<Interval>& nodeIntervals = data[nodeKey].intervals;
    if(data[nodeKey].keyframeIdcs.empty())
    {
        keyframes.push_back({0.0f, 0.0f});
        data[nodeKey].keyframeIdcs.push_back(keyframes.size() - 1);
    }

    data[nodeKey].keyframeIdcs.push_back(keyframes.size());
    keyframes.push_back(key);

    findCorrectKeyframe(nodeKey);
}
