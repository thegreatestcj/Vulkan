// src/culling/frustum.cpp
#include "frustum.h"

void Frustum::extract(const glm::mat4& vp) {
    // Left: row3 + row0
    planes_[0] = glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]
    );
    // Right: row3 - row0
    planes_[1] = glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]
    );
    // Bottom: row3 + row1
    planes_[2] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]
    );
    // Top: row3 - row1
    planes_[3] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]
    );
    // Near (Vulkan z >= 0): row2
    planes_[4] = glm::vec4(
        vp[0][2],
        vp[1][2],
        vp[2][2],
        vp[3][2]
    );
    // Far: row3 - row2
    planes_[5] = glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]
    );

    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float len = glm::length(glm::vec3(planes_[i]));
        planes_[i] /= len;
    }
}

bool Frustum::testSphere(const glm::vec3& center, float radius) const {
    for (int i = 0; i < 6; i++) {
        float dist = glm::dot(glm::vec3(planes_[i]), center) + planes_[i].w;
        if (dist < -radius) {
            return false;
        }
    }
    return true;
}

bool Frustum::testAABB(const glm::vec3& aabbMin, const glm::vec3& aabbMax) const {
    for (int i = 0; i < 6; i++) {
        // P-vertex: corner most aligned with plane normal
        glm::vec3 pVertex;
        pVertex.x = (planes_[i].x > 0) ? aabbMax.x : aabbMin.x;
        pVertex.y = (planes_[i].y > 0) ? aabbMax.y : aabbMin.y;
        pVertex.z = (planes_[i].z > 0) ? aabbMax.z : aabbMin.z;

        float dist = glm::dot(glm::vec3(planes_[i]), pVertex) + planes_[i].w;
        if (dist < 0) {
            return false;
        }
    }
    return true;
}