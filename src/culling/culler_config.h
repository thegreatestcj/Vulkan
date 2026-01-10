// src/culling/culler_config.h
#pragma once

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <fmt/core.h>

enum class CullingMode {
    CPU_None,
    CPU_Plane,
    CPU_BVH,
    GPU_Plane
};

struct CullingConfig {
    CullingMode mode = CullingMode::CPU_None;
    uint32_t bvhLeafSize = 4;

    static CullingConfig load(const std::string& path) {
        CullingConfig config;

        std::ifstream file(path);
        if (!file.is_open()) {
            fmt::print("Warning: Could not open {}, using defaults\n", path);
            return config;
        }

        nlohmann::json j;
        file >> j;

        std::string modeStr = j.value("culling_mode", "cpu_none");
        if (modeStr == "cpu_none") config.mode = CullingMode::CPU_None;
        else if (modeStr == "cpu_plane") config.mode = CullingMode::CPU_Plane;
        else if (modeStr == "cpu_bvh") config.mode = CullingMode::CPU_BVH;
        else if (modeStr == "gpu_plane") config.mode = CullingMode::GPU_Plane;

        config.bvhLeafSize = j.value("bvh_leaf_size", 4);

        return config;
    }
};