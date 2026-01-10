// src/culling/culler_factory.cpp
#include "culler_factory.h"
#include "cpu_culler_none.h"
#include "cpu_culler_plane.h"
#include "cpu_culler_bvh.h"
// #include "gpu_culler_plane.h"  // TODO: implement later

std::unique_ptr<ICuller> CullerFactory::create(const CullingConfig& config, VulkanEngine* engine) {
    switch (config.mode) {
        case CullingMode::CPU_None:
            return std::make_unique<CPUCullerNone>();

        case CullingMode::CPU_Plane:
            return std::make_unique<CPUCullerPlane>();

        case CullingMode::CPU_BVH:
            return std::make_unique<CPUCullerBVH>(config.bvhLeafSize);

        case CullingMode::GPU_Plane:
            // TODO: implement
                fmt::print("GPU culling not yet implemented, falling back to CPU_BVH\n");
        return std::make_unique<CPUCullerBVH>(config.bvhLeafSize);

        default:
            return std::make_unique<CPUCullerNone>();
    }
}