// src/culling/cpu_culler_plane.h
#pragma once

#include "culler_interface.h"
#include "frustum.h"

class CPUCullerPlane : public ICuller {
public:
    void cull(const std::vector<RenderObject>& objects,
              const glm::mat4& viewproj,
              std::vector<uint32_t>& visibleIndices) override;

private:
    Frustum frustum_;
};