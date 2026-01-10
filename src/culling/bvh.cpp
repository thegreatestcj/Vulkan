// src/culling/bvh.cpp
#include "bvh.h"
#include <vk_engine.h>
#include <vk_types.h>
#include <algorithm>
#include <cfloat>

void BVH::build(const std::vector<RenderObject>& objects, uint32_t leafSize) {
    clear();
    if (objects.empty()) return;

    leafSize_ = leafSize;

    // Initialize object indices
    objectIndices_.resize(objects.size());
    for (uint32_t i = 0; i < objects.size(); i++) {
        objectIndices_[i] = i;
    }

    // Reserve nodes (worst case: 2N - 1)
    nodes_.reserve(2 * objects.size() - 1);

    // Build recursively
    buildRecursive(objects, 0, objects.size());
}

void BVH::clear() {
    nodes_.clear();
    objectIndices_.clear();
}

uint32_t BVH::buildRecursive(const std::vector<RenderObject>& objects,
                              uint32_t start, uint32_t end) {
    uint32_t nodeIndex = nodes_.size();
    nodes_.push_back({});
    BVHNode& node = nodes_[nodeIndex];

    // Calculate AABB for this node
    node.aabbMin = glm::vec3(FLT_MAX);
    node.aabbMax = glm::vec3(-FLT_MAX);

    for (uint32_t i = start; i < end; i++) {
        const auto& obj = objects[objectIndices_[i]];

        // Transform bounds to world space (approximate AABB)
        glm::vec3 worldCenter = glm::vec3(obj.transform * glm::vec4(obj.bounds.origin, 1.0f));
        float maxScale = glm::max(
            glm::length(glm::vec3(obj.transform[0])),
            glm::max(
                glm::length(glm::vec3(obj.transform[1])),
                glm::length(glm::vec3(obj.transform[2]))
            )
        );
        glm::vec3 worldExtents = obj.bounds.extents * maxScale;

        glm::vec3 objMin = worldCenter - worldExtents;
        glm::vec3 objMax = worldCenter + worldExtents;

        node.aabbMin = glm::min(node.aabbMin, objMin);
        node.aabbMax = glm::max(node.aabbMax, objMax);
    }

    uint32_t objectCount = end - start;

    // Leaf node
    if (objectCount <= leafSize_) {
        node.firstObjectIndex = start;
        node.objectCount = objectCount;
        return nodeIndex;
    }

    // Find best split axis (largest extent)
    glm::vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    // Split at median
    uint32_t mid = (start + end) / 2;
    std::nth_element(
        objectIndices_.begin() + start,
        objectIndices_.begin() + mid,
        objectIndices_.begin() + end,
        [&](uint32_t a, uint32_t b) {
            glm::vec3 centerA = glm::vec3(objects[a].transform * glm::vec4(objects[a].bounds.origin, 1.0f));
            glm::vec3 centerB = glm::vec3(objects[b].transform * glm::vec4(objects[b].bounds.origin, 1.0f));
            return centerA[axis] < centerB[axis];
        }
    );

    // Mark as internal node
    node.objectCount = 0;

    // Build children (store leftChild first, rightChild = leftChild + 1 is implicit)
    node.leftChild = buildRecursive(objects, start, mid);
    node.rightChild = buildRecursive(objects, mid, end);

    return nodeIndex;
}