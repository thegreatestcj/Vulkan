// src/culling/culler_interface.h
#pragma once

#include <vk_types.h>
#include <vector>
#include <glm/glm.hpp>

struct RenderObject;  // Forward declaration

struct CullStats {
    uint32_t totalObjects = 0;
    uint32_t visibleObjects = 0;
    uint32_t culledObjects = 0;
    float cullTimeMs = 0.0f;
};

class ICuller {
public:
    virtual ~ICuller() = default;

    // Called when scene changes (optional, for BVH rebuild)
    virtual void onSceneChanged(const std::vector<RenderObject>& objects) {}

    // Perform culling, returns indices of visible objects
    virtual void cull(const std::vector<RenderObject>& objects,
                      const glm::mat4& viewproj,
                      std::vector<uint32_t>& visibleIndices) = 0;

    // For GPU culler
    virtual bool isGPUBased() const { return false; }
    virtual VkBuffer getIndirectBuffer() const { return VK_NULL_HANDLE; }
    virtual VkBuffer getCountBuffer() const { return VK_NULL_HANDLE; }

    // Stats
    const CullStats& getStats() const { return stats_; }

protected:
    CullStats stats_;
};