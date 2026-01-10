// src/culling/cpu_culler_none.cpp
#include "cpu_culler_none.h"
#include <vk_types.h>
#include <chrono>

// Original is_visible from vk_engine.cpp
static bool is_visible_ndc(const RenderObject& obj, const glm::mat4& viewproj) {
    std::array<glm::vec3, 8> corners {
        glm::vec3 { 1, 1, 1 },
        glm::vec3 { 1, 1, -1 },
        glm::vec3 { 1, -1, 1 },
        glm::vec3 { 1, -1, -1 },
        glm::vec3 { -1, 1, 1 },
        glm::vec3 { -1, 1, -1 },
        glm::vec3 { -1, -1, 1 },
        glm::vec3 { -1, -1, -1 },
    };

    glm::mat4 matrix = viewproj * obj.transform;
    glm::vec3 min = { 1.5f, 1.5f, 1.5f };
    glm::vec3 max = { -1.5f, -1.5f, -1.5f };

    for (int c = 0; c < 8; c++) {
        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);
        v.x = v.x / v.w;
        v.y = v.y / v.w;
        v.z = v.z / v.w;
        min = glm::min(glm::vec3 { v.x, v.y, v.z }, min);
        max = glm::max(glm::vec3 { v.x, v.y, v.z }, max);
    }

    if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
        return false;
    }
    return true;
}

void CPUCullerNone::cull(const std::vector<RenderObject>& objects,
                         const glm::mat4& viewproj,
                         std::vector<uint32_t>& visibleIndices) {
    auto start = std::chrono::high_resolution_clock::now();

    visibleIndices.clear();
    visibleIndices.reserve(objects.size());

    for (uint32_t i = 0; i < objects.size(); i++) {
        if (is_visible_ndc(objects[i], viewproj)) {
            visibleIndices.push_back(i);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    stats_.totalObjects = objects.size();
    stats_.visibleObjects = visibleIndices.size();
    stats_.culledObjects = stats_.totalObjects - stats_.visibleObjects;
    stats_.cullTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
}