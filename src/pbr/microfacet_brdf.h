/**
 * @file microfacet_brdf.h
 * @brief Microfacet BRDF implementation using Cook-Torrance model
 */

#pragma once

#include "material_shader_registry.h"
#include <glm/glm.hpp>

// ============================================================================
// Microfacet Material Constants (256 bytes)
// ============================================================================

struct alignas(16) MicrofacetMaterialConstants {
    // Base parameters
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

    // Microfacet extensions
    float anisotropy;
    float anisotropyRotation;
    float ior;
    float specularTint;

    float clearcoat;
    float clearcoatRoughness;
    float sheen;
    float sheenTint;

    uint32_t anisotropyTexID;
    uint32_t clearcoatTexID;
    uint32_t clearcoatRoughnessTexID;
    uint32_t sheenTexID;

    uint32_t occlusionTexID;
    uint32_t _pad1;
    uint32_t _pad2;
    uint32_t _pad3;

    // Padding to 256 bytes
    glm::vec4 _reserved[7];
};

static_assert(sizeof(MicrofacetMaterialConstants) == 256,
    "MicrofacetMaterialConstants must be 256 bytes");

// ============================================================================
// Microfacet BRDF Implementation
// ============================================================================

class MicrofacetBRDF : public IMaterialShader {
public:
    MicrofacetBRDF() = default;
    ~MicrofacetBRDF() override = default;

    ShadingModel get_shading_model() const override {
        return ShadingModel::MICROFACET_BRDF;
    }

    const char* get_name() const override {
        return "Microfacet BRDF (Cook-Torrance)";
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
        return sizeof(MicrofacetMaterialConstants);
    }

    // Microfacet-specific methods
    MaterialInstance write_microfacet_material(
        VkDevice device,
        MaterialPass pass,
        const MicrofacetMaterialConstants& constants,
        VkBuffer dataBuffer,
        uint32_t dataBufferOffset,
        DescriptorAllocatorGrowable& descriptorAllocator
    );

    static MicrofacetMaterialConstants to_microfacet_constants(
        const BaseMaterialConstants& base
    );

private:
    MaterialPipeline _opaquePipeline{};
    MaterialPipeline _transparentPipeline{};
    VkDescriptorSetLayout _materialLayout = VK_NULL_HANDLE;
    DescriptorWriter _writer;
};