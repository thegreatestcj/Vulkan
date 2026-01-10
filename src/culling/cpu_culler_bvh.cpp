// src/culling/cpu_culler_bvh.cpp
#include "cpu_culler_bvh.h"
#include <vk_types.h>
#include <vk_engine.h>
#include <chrono>

void CPUCullerBVH::onSceneChanged(const std::vector<RenderObject>& objects) {
    needsRebuild_ = true;
}

void CPUCullerBVH::cull(const std::vector<RenderObject>& objects,
                        const glm::mat4& viewproj,
                        std::vector<uint32_t>& visibleIndices) {
    auto start = std::chrono::high_resolution_clock::now();

    // Rebuild BVH if needed
    if (needsRebuild_ || bvh_.isEmpty()) {
        bvh_.build(objects, leafSize_);
        needsRebuild_ = false;
    }

    frustum_.extract(viewproj);

    visibleIndices.clear();
    visibleIndices.reserve(objects.size());

    if (!bvh_.isEmpty()) {
        cullRecursive(0, objects, visibleIndices);
    }

    auto end = std::chrono::high_resolution_clock::now();

    stats_.totalObjects = objects.size();
    stats_.visibleObjects = visibleIndices.size();
    stats_.culledObjects = stats_.totalObjects - stats_.visibleObjects;
    stats_.cullTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
}

void CPUCullerBVH::cullRecursive(uint32_t nodeIndex,
                                  const std::vector<RenderObject>& objects,
                                  std::vector<uint32_t>& visibleIndices) {
    const auto& nodes = bvh_.getNodes();
    const auto& objectIndices = bvh_.getObjectIndices();
    const BVHNode& node = nodes[nodeIndex];

    // Test node AABB against frustum
    if (!frustum_.testAABB(node.aabbMin, node.aabbMax)) {
        return;  // Cull entire subtree
    }

    if (node.isLeaf()) {
        // Test individual objects in leaf
        for (uint32_t i = 0; i < node.objectCount; i++) {
            uint32_t objIdx = objectIndices[node.firstObjectIndex + i];
            const auto& obj = objects[objIdx];

            // Final sphere test
            glm::vec3 worldCenter = glm::vec3(obj.transform * glm::vec4(obj.bounds.origin, 1.0f));
            float maxScale = glm::max(
                glm::length(glm::vec3(obj.transform[0])),
                glm::max(
                    glm::length(glm::vec3(obj.transform[1])),
                    glm::length(glm::vec3(obj.transform[2]))
                )
            );
            float worldRadius = obj.bounds.sphereRadius * maxScale;

            if (frustum_.testSphere(worldCenter, worldRadius)) {
                visibleIndices.push_back(objIdx);
            }
        }
    } else {
        // Recurse into children
        cullRecursive(node.leftChild, objects, visibleIndices);
        cullRecursive(node.rightChild, objects, visibleIndices);
    }
}