// src/culling/cpu_culler_none.h
#pragma once

#include "culler_interface.h"

class CPUCullerNone : public ICuller {
public:
    void cull(const std::vector<RenderObject>& objects,
              const glm::mat4& viewproj,
              std::vector<uint32_t>& visibleIndices) override;
};