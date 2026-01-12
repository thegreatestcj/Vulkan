/**
 * @file disney_brdf.h
 * @brief Disney Principled BRDF/BSDF implementation
 */

#pragma once

#include "material_shader_registry.h"
#include <glm/glm.hpp>

// ============================================================================
// Disney Material Flags
// ============================================================================

namespace DisneyFlags {
    constexpr uint32_t DOUBLE_SIDED       = 1 << 0;
    constexpr uint32_t THIN_WALLED        = 1 << 1;
    constexpr uint32_t USE_SUBSURFACE_TEX = 1 << 2;
    constexpr uint32_t USE_THIN_FILM      = 1 << 3;
    constexpr uint32_t ENERGY_COMPENSATE  = 1 << 4;
}

// ============================================================================
// Disney Material Constants (256 bytes)
// ============================================================================

struct alignas(16) DisneyMaterialConstants {
    glm::vec4 baseColor;

    float metallic;
    float roughness;
    float subsurface;
    float specular;

    float specularTint;
    float anisotropic;
    float sheen;
    float sheenTint;

    float clearcoat;
    float clearcoatGloss;
    float transmission;
    float transmissionRoughness;

    float ior;
    float anisotropicRotation;
    float emissiveStrength;
    float normalScale;

    float occlusionStrength;
    float alphaCutoff;
    uint32_t alphaMode;
    uint32_t flags;

    glm::vec4 emissiveColor;

    uint32_t baseColorTexID;
    uint32_t normalTexID;
    uint32_t metallicRoughnessTexID;
    uint32_t emissiveTexID;

    uint32_t occlusionTexID;
    uint32_t specularTexID;
    uint32_t transmissionTexID;
    uint32_t sheenTexID;

    uint32_t clearcoatTexID;
    uint32_t clearcoatRoughnessTexID;
    uint32_t anisotropyTexID;
    uint32_t subsurfaceColorTexID;

    glm::vec3 subsurfaceColor;
    float subsurfaceRadius;

    float thinFilmThickness;
    float thinFilmIOR;
    uint32_t _reserved0;
    uint32_t _reserved1;

    glm::vec4 _padding[3];
};

static_assert(sizeof(DisneyMaterialConstants) == 256,
    "DisneyMaterialConstants must be 256 bytes");

// ============================================================================
// Disney BRDF Implementation
// ============================================================================

class DisneyBRDF : public IMaterialShader {
public:
    explicit DisneyBRDF(bool enableTransmission = false);
    ~DisneyBRDF() override = default;

    ShadingModel get_shading_model() const override {
        return _enableTransmission
            ? ShadingModel::DISNEY_BSDF
            : ShadingModel::DISNEY_PRINCIPLED;
    }

    const char* get_name() const override {
        return _enableTransmission
            ? "Disney BSDF (with Transmission)"
            : "Disney Principled BRDF";
    }

    void build_pipelines(VulkanEngine* engine) override;
    void clear_resources(VkDevice device) override;

    MaterialInstance write_material(
        VkDevice device,
        MaterialPass pass,
        const BaseMaterialConstants& baseConstants,
        VkBuffer dataBuffer,
        uint32_t dataBufferOffset,
        DescriptorAllocatorGrowable& descriptorAllocator
    ) override;

    MaterialPipeline* get_pipeline(MaterialPass pass) override {
        return (pass == MaterialPass::Transparent)
            ? &_transparentPipeline
            : &_opaquePipeline;
    }

    VkDescriptorSetLayout get_material_layout() const override {
        return _materialLayout;
    }

    bool supports_feature(const char* featureName) const override;

    size_t get_constants_size() const override {
        return sizeof(DisneyMaterialConstants);
    }

    // Disney-specific methods
    MaterialInstance write_disney_material(
        VkDevice device,
        MaterialPass pass,
        const DisneyMaterialConstants& constants,
        VkBuffer dataBuffer,
        uint32_t dataBufferOffset,
        DescriptorAllocatorGrowable& descriptorAllocator
    );

    static DisneyMaterialConstants to_disney_constants(const BaseMaterialConstants& base);
    static DisneyMaterialConstants create_default();
    static DisneyMaterialConstants create_metal(const glm::vec3& color, float roughness = 0.3f);
    static DisneyMaterialConstants create_glass(float ior = 1.5f, float roughness = 0.0f,
                                                 const glm::vec3& tint = glm::vec3(1.0f));
    static DisneyMaterialConstants create_subsurface(const glm::vec3& baseColor,
                                                      const glm::vec3& subsurfaceColor,
                                                      float subsurface = 0.5f);

private:
    MaterialPipeline _opaquePipeline{};
    MaterialPipeline _transparentPipeline{};
    VkDescriptorSetLayout _materialLayout = VK_NULL_HANDLE;
    DescriptorWriter _writer;
    bool _enableTransmission;
};