/**
 * @file material_shader_interface.h
 * @brief Base interface for all material shader implementations (BRDF/BSDF)
 */

#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <unordered_map>

// Forward declarations
class VulkanEngine;

// ============================================================================
// Shading Model Enumeration
// ============================================================================

enum class ShadingModel : uint8_t {
    METALLIC_ROUGHNESS = 0,  // Original glTF PBR (simplified)
    MICROFACET_BRDF,         // Classic Cook-Torrance microfacet
    DISNEY_PRINCIPLED,       // Disney Principled BRDF
    DISNEY_BSDF,             // Disney BSDF with transmission
};

inline const char* shading_model_name(ShadingModel model) {
    switch (model) {
        case ShadingModel::METALLIC_ROUGHNESS: return "Metallic-Roughness";
        case ShadingModel::MICROFACET_BRDF:    return "Microfacet BRDF";
        case ShadingModel::DISNEY_PRINCIPLED:  return "Disney Principled";
        case ShadingModel::DISNEY_BSDF:        return "Disney BSDF";
        default: return "Unknown";
    }
}

// ============================================================================
// Base Material Constants
// ============================================================================

struct alignas(16) BaseMaterialConstants {
    glm::vec4 baseColorFactor;
    glm::vec4 emissiveFactor;

    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;

    uint32_t baseColorTexID;
    uint32_t normalTexID;
    uint32_t metallicRoughnessTexID;
    uint32_t emissiveTexID;

    float alphaCutoff;
    uint32_t alphaMode;
    uint32_t doubleSided;
    uint32_t _pad0;
};

static_assert(sizeof(BaseMaterialConstants) == 80,
    "BaseMaterialConstants must be 80 bytes");

// ============================================================================
// IMaterialShader Interface
// ============================================================================

class IMaterialShader {
public:
    virtual ~IMaterialShader() = default;

    virtual ShadingModel get_shading_model() const = 0;
    virtual const char* get_name() const = 0;

    virtual void build_pipelines(VulkanEngine* engine) = 0;
    virtual void clear_resources(VkDevice device) = 0;

    virtual MaterialInstance write_material(
        VkDevice device,
        MaterialPass pass,
        const BaseMaterialConstants& baseConstants,
        VkBuffer dataBuffer,
        uint32_t dataBufferOffset,
        DescriptorAllocatorGrowable& descriptorAllocator
    ) = 0;

    virtual MaterialPipeline* get_pipeline(MaterialPass pass) = 0;
    virtual VkDescriptorSetLayout get_material_layout() const = 0;

    virtual bool supports_feature(const char* featureName) const {
        (void)featureName;
        return false;
    }

    virtual size_t get_constants_size() const {
        return sizeof(BaseMaterialConstants);
    }
};

// ============================================================================
// Shader Registry
// ============================================================================

class ShaderRegistry {
public:
    static void init(VulkanEngine* engine);
    static void cleanup(VkDevice device);
    static IMaterialShader* get(ShadingModel model);
    static void register_shader(std::unique_ptr<IMaterialShader> shader);

    static ShadingModel get_default_model() { return _defaultModel; }
    static void set_default_model(ShadingModel model) { _defaultModel = model; }

    static const std::unordered_map<ShadingModel, std::unique_ptr<IMaterialShader>>&
    get_all_shaders() { return _shaders; }

private:
    static std::unordered_map<ShadingModel, std::unique_ptr<IMaterialShader>> _shaders;
    static ShadingModel _defaultModel;
};