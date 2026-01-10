// src/culling/cpu_culler_plane.cpp
#include "cpu_culler_plane.h"
#include <vk_types.h>
#include <vk_engine.h>
#include <chrono>

void CPUCullerPlane::cull(const std::vector<RenderObject>& objects,
                          const glm::mat4& viewproj,
                          std::vector<uint32_t>& visibleIndices) {
    auto start = std::chrono::high_resolution_clock::now();

    frustum_.extract(viewproj);

    visibleIndices.clear();
    visibleIndices.reserve(objects.size());

    for (uint32_t i = 0; i < objects.size(); i++) {
        const auto& obj = objects[i];

        // Transform bounds to world space
        glm::vec3 worldCenter = glm::vec3(obj.transform * glm::vec4(obj.bounds.origin, 1.0f));

        // Approximate world-space radius (conservative)
        float maxScale = glm::max(
            glm::length(glm::vec3(obj.transform[0])),
            glm::max(
                glm::length(glm::vec3(obj.transform[1])),
                glm::length(glm::vec3(obj.transform[2]))
            )
        );
        float worldRadius = obj.bounds.sphereRadius * maxScale;

        if (frustum_.testSphere(worldCenter, worldRadius)) {
            visibleIndices.push_back(i);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    stats_.totalObjects = objects.size();
    stats_.visibleObjects = visibleIndices.size();
    stats_.culledObjects = stats_.totalObjects - stats_.visibleObjects;
    stats_.cullTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
}