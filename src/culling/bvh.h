// src/culling/bvh.h
#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct RenderObject;

struct BVHNode {
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;

    union {
        struct { uint32_t leftChild; uint32_t rightChild; };
        struct { uint32_t firstObjectIndex; uint32_t objectCount; };
    };

    bool isLeaf() const { return objectCount > 0; }
};

class BVH {
public:
    void build(const std::vector<RenderObject>& objects, uint32_t leafSize = 4);
    void clear();

    bool isEmpty() const { return nodes_.empty(); }
    const std::vector<BVHNode>& getNodes() const { return nodes_; }
    const std::vector<uint32_t>& getObjectIndices() const { return objectIndices_; }

private:
    std::vector<BVHNode> nodes_;
    std::vector<uint32_t> objectIndices_;
    uint32_t leafSize_ = 4;

    uint32_t buildRecursive(const std::vector<RenderObject>& objects,
                            uint32_t start, uint32_t end);
};