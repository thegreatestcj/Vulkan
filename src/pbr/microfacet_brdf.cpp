/**
 * @file microfacet_brdf.cpp
 * @brief Implementation of Cook-Torrance Microfacet BRDF
 */

#include "microfacet_brdf.h"
#include "vk_engine.h"
#include "vk_pipelines.h"
#include "vk_initializers.h"
#include <fmt/core.h>
#include <cstring>

void MicrofacetBRDF::build_pipelines(VulkanEngine* engine) {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    if (!vkutil::load_shader_module("../shaders/mesh.vert.spv",
                                     engine->_device, &vertShader)) {
        fmt::println("Error: Failed to load vertex shader for MicrofacetBRDF");
        return;
    }

    if (!vkutil::load_shader_module("../shaders/microfacet.frag.spv",
                                     engine->_device, &fragShader)) {
        fmt::println("Error: Failed to load microfacet fragment shader");
        vkDestroyShaderModule(engine->_device, vertShader, nullptr);
        return;
    }

    // Descriptor Layout
    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    _materialLayout = layoutBuilder.build(
        engine->_device,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );

    // Pipeline Layout
    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[] = {
        engine->_gpuSceneDataDescriptorLayout,
        _materialLayout
    };

    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
    layoutInfo.setLayoutCount = 2;
    layoutInfo.pSetLayouts = layouts;
    layoutInfo.pPushConstantRanges = &matrixRange;
    layoutInfo.pushConstantRangeCount = 1;

    VkPipelineLayout pipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(engine->_device, &layoutInfo,
                                     nullptr, &pipelineLayout));

    _opaquePipeline.layout = pipelineLayout;
    _transparentPipeline.layout = pipelineLayout;

    // Build Opaque Pipeline
    PipelineBuilder builder;

    builder.set_shaders(vertShader, fragShader);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    builder.set_multisampling_none();
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.set_color_attachment_format(engine->_drawImage.imageFormat);
    builder.set_depth_format(engine->_depthImage.imageFormat);
    builder._pipelineLayout = pipelineLayout;

    _opaquePipeline.pipeline = builder.build_pipeline(engine->_device);

    // Build Transparent Pipeline
    builder.enable_blending_additive();
    builder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    _transparentPipeline.pipeline = builder.build_pipeline(engine->_device);

    vkDestroyShaderModule(engine->_device, vertShader, nullptr);
    vkDestroyShaderModule(engine->_device, fragShader, nullptr);

    fmt::println("MicrofacetBRDF: Pipelines built successfully");
}

void MicrofacetBRDF::clear_resources(VkDevice device) {
    if (_opaquePipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _opaquePipeline.pipeline, nullptr);
        _opaquePipeline.pipeline = VK_NULL_HANDLE;
    }

    if (_transparentPipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _transparentPipeline.pipeline, nullptr);
        _transparentPipeline.pipeline = VK_NULL_HANDLE;
    }

    if (_opaquePipeline.layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _opaquePipeline.layout, nullptr);
        _opaquePipeline.layout = VK_NULL_HANDLE;
        _transparentPipeline.layout = VK_NULL_HANDLE;
    }

    if (_materialLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _materialLayout, nullptr);
        _materialLayout = VK_NULL_HANDLE;
    }

    fmt::println("MicrofacetBRDF: Resources cleaned up");
}

MaterialInstance MicrofacetBRDF::write_material(
    VkDevice device,
    MaterialPass pass,
    const BaseMaterialConstants& baseConstants,
    VkBuffer dataBuffer,
    uint32_t dataBufferOffset,
    DescriptorAllocatorGrowable& descriptorAllocator
) {
    MicrofacetMaterialConstants constants = to_microfacet_constants(baseConstants);

    return write_microfacet_material(
        device, pass, constants,
        dataBuffer, dataBufferOffset,
        descriptorAllocator
    );
}

MaterialInstance MicrofacetBRDF::write_microfacet_material(
    VkDevice device,
    MaterialPass pass,
    const MicrofacetMaterialConstants& constants,
    VkBuffer dataBuffer,
    uint32_t dataBufferOffset,
    DescriptorAllocatorGrowable& descriptorAllocator
) {
    MaterialInstance instance;
    instance.passType = pass;
    instance.pipeline = (pass == MaterialPass::Transparent)
        ? &_transparentPipeline
        : &_opaquePipeline;

    instance.materialSet = descriptorAllocator.allocate(device, _materialLayout);

    _writer.clear();
    _writer.write_buffer(
        0,
        dataBuffer,
        sizeof(MicrofacetMaterialConstants),
        dataBufferOffset,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    );
    _writer.update_set(device, instance.materialSet);

    return instance;
}

MicrofacetMaterialConstants MicrofacetBRDF::to_microfacet_constants(
    const BaseMaterialConstants& base
) {
    MicrofacetMaterialConstants result{};

    result.baseColorFactor = base.baseColorFactor;
    result.emissiveFactor = base.emissiveFactor;
    result.metallicFactor = base.metallicFactor;
    result.roughnessFactor = base.roughnessFactor;
    result.normalScale = base.normalScale;
    result.occlusionStrength = base.occlusionStrength;
    result.baseColorTexID = base.baseColorTexID;
    result.normalTexID = base.normalTexID;
    result.metallicRoughnessTexID = base.metallicRoughnessTexID;
    result.emissiveTexID = base.emissiveTexID;
    result.alphaCutoff = base.alphaCutoff;
    result.alphaMode = base.alphaMode;
    result.doubleSided = base.doubleSided;

    // Microfacet defaults
    result.anisotropy = 0.0f;
    result.anisotropyRotation = 0.0f;
    result.ior = 1.5f;
    result.specularTint = 0.0f;
    result.clearcoat = 0.0f;
    result.clearcoatRoughness = 0.03f;
    result.sheen = 0.0f;
    result.sheenTint = 0.5f;

    result.anisotropyTexID = UINT32_MAX;
    result.clearcoatTexID = UINT32_MAX;
    result.clearcoatRoughnessTexID = UINT32_MAX;
    result.sheenTexID = UINT32_MAX;
    result.occlusionTexID = UINT32_MAX;

    return result;
}

bool MicrofacetBRDF::supports_feature(const char* featureName) const {
    if (strcmp(featureName, "anisotropy") == 0) return true;
    if (strcmp(featureName, "clearcoat") == 0) return true;
    if (strcmp(featureName, "sheen") == 0) return true;
    if (strcmp(featureName, "ior") == 0) return true;
    return false;
}