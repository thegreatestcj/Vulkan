// src/culling/cpu_culler_bvh.h
#pragma once

#include "culler_interface.h"
#include "frustum.h"
#include "bvh.h"

class CPUCullerBVH : public ICuller {
public:
    explicit CPUCullerBVH(uint32_t leafSize = 4) : leafSize_(leafSize) {}

    void onSceneChanged(const std::vector<RenderObject>& objects) override;

    void cull(const std::vector<RenderObject>& objects,
              const glm::mat4& viewproj,
              std::vector<uint32_t>& visibleIndices) override;

private:
    uint32_t leafSize_;
    BVH bvh_;
    Frustum frustum_;
    bool needsRebuild_ = true;

    void cullRecursive(uint32_t nodeIndex,
                       const std::vector<RenderObject>& objects,
                       std::vector<uint32_t>& visibleIndices);
};