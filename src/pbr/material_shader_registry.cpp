/**
 * @file shader_registry.cpp
 * @brief Implementation of ShaderRegistry
 */

#include "material_shader_registry.h"
#include "microfacet_brdf.h"
#include "disney_brdf.h"
#include <fmt/core.h>

// Static member definitions
std::unordered_map<ShadingModel, std::unique_ptr<IMaterialShader>> ShaderRegistry::_shaders;
ShadingModel ShaderRegistry::_defaultModel = ShadingModel::METALLIC_ROUGHNESS;

void ShaderRegistry::init(VulkanEngine* engine) {
    fmt::println("========================================");
    fmt::println("Initializing ShaderRegistry...");
    fmt::println("========================================");

    // Microfacet BRDF (Cook-Torrance)
    {
        auto shader = std::make_unique<MicrofacetBRDF>();
        shader->build_pipelines(engine);
        _shaders[ShadingModel::MICROFACET_BRDF] = std::move(shader);
        fmt::println("  [+] Registered: Microfacet BRDF (Cook-Torrance)");
    }

    // Disney Principled BRDF
    {
        auto shader = std::make_unique<DisneyBRDF>(false);
        shader->build_pipelines(engine);
        _shaders[ShadingModel::DISNEY_PRINCIPLED] = std::move(shader);
        fmt::println("  [+] Registered: Disney Principled BRDF");
    }

    // Disney BSDF (with transmission)
    {
        auto shader = std::make_unique<DisneyBRDF>(true);
        shader->build_pipelines(engine);
        _shaders[ShadingModel::DISNEY_BSDF] = std::move(shader);
        fmt::println("  [+] Registered: Disney BSDF (with Transmission)");
    }

    fmt::println("========================================");
    fmt::println("ShaderRegistry: {} shaders registered", _shaders.size());
    fmt::println("========================================");
}

void ShaderRegistry::cleanup(VkDevice device) {
    fmt::println("ShaderRegistry: Cleaning up {} shaders...", _shaders.size());

    for (auto& [model, shader] : _shaders) {
        if (shader) {
            fmt::println("  [-] Cleaning: {}", shader->get_name());
            shader->clear_resources(device);
        }
    }

    _shaders.clear();
    fmt::println("ShaderRegistry: Cleanup complete");
}

IMaterialShader* ShaderRegistry::get(ShadingModel model) {
    auto it = _shaders.find(model);
    if (it != _shaders.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ShaderRegistry::register_shader(std::unique_ptr<IMaterialShader> shader) {
    if (!shader) {
        fmt::println("Warning: Attempted to register null shader");
        return;
    }

    ShadingModel model = shader->get_shading_model();

    if (_shaders.find(model) != _shaders.end()) {
        fmt::println("Warning: Overwriting existing shader: {}",
                     shading_model_name(model));
    }

    fmt::println("ShaderRegistry: Registered custom shader: {}", shader->get_name());
    _shaders[model] = std::move(shader);
}