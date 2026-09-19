#pragma once
#include <core/Utils.h>
#include "core/Scene.h"

#include <unordered_map>
const uint32_t MAX_SAMPLES_SHADER = 1;


struct LinearFunc
{
    float a;
    float b;
};

enum Variable
{
    POSITION,
    ROTATION,
    SCALE
};
struct Keyframe
{
    float time;
    float fvalue;
};

struct Interval
{
    uint32_t iKeyframeIdx;
    uint32_t fKeyframeIdx;
    LinearFunc func;
};

struct nodeKeyData
{
    uint32_t modelId;
    uint32_t nodeId;
};

inline nodeKeyData getNodeFromkey(uint64_t key)
{
    uint32_t modelId = key >> 32;
    uint32_t nodeId  = key & 0xFFFFFFFFu;
    return {modelId, nodeId};
}

inline uint64_t makeNodeKey(uint32_t modelId, uint32_t nodeId)
{
    return (static_cast<uint64_t>(modelId) << 32) | nodeId;
}

struct NodeVariableKey
{
    uint32_t modelId;
    uint32_t nodeId;
    Variable var;     // 0..3
    uint32_t varIdx;

    bool operator==(const NodeVariableKey& other) const
    {
        return modelId == other.modelId &&
               nodeId  == other.nodeId &&
               var     == other.var &&
               varIdx  == other.varIdx;
    }
};
struct NodeVariableKeyHash
{
    size_t operator()(const NodeVariableKey& k) const noexcept
    {
        size_t h = std::hash<uint32_t>{}(k.modelId);

        h ^= std::hash<uint32_t>{}(k.nodeId) +
             0x9e3779b9 + (h << 6) + (h >> 2);

        h ^= std::hash<uint32_t>{}(k.var) +
             0x9e3779b9 + (h << 6) + (h >> 2);

        h ^= std::hash<uint32_t>{}(k.varIdx) +
             0x9e3779b9 + (h << 6) + (h >> 2);

        return h;
    }
};
struct animData
{
    std::vector<Interval> intervals;
    std::vector<uint32_t> keyframeIdcs;
};
class Animator
{
    public:
        void run(uint32_t frame, Scene& scene);
        uint32_t animationFrame = 0;
        uint32_t renderFramesPerAnimFrame = 100;
        void addKeyframe(Keyframe key, NodeVariableKey nodeKey);
        float timeStep = 24;
        std::vector<Keyframe> keyframes;
    private:

        void findCorrectKeyframe(NodeVariableKey nodeKey);
        std::unordered_map<NodeVariableKey, animData, NodeVariableKeyHash> data;
};