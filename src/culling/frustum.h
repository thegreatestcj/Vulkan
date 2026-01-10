// src/culling/frustum.h
#pragma once

#include <glm/glm.hpp>
#include <array>

struct Bounds;

class Frustum {
public:
    void extract(const glm::mat4& viewproj);

    bool testSphere(const glm::vec3& center, float radius) const;
    bool testAABB(const glm::vec3& min, const glm::vec3& max) const;

private:
    std::array<glm::vec4, 6> planes_;  // left, right, bottom, top, near, far
};