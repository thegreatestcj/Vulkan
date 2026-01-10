// src/culling/culler_factory.h
#pragma once

#include "culler_interface.h"
#include "culler_config.h"
#include <memory>

class VulkanEngine;

class CullerFactory {
public:
    static std::unique_ptr<ICuller> create(const CullingConfig& config, VulkanEngine* engine = nullptr);
};