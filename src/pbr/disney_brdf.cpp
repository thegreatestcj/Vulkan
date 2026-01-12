/**
 * @file disney_brdf.cpp
 * @brief Implementation of Disney Principled BRDF/BSDF
 */

#include "disney_brdf.h"
#include "vk_engine.h"
#include "vk_pipelines.h"
#include "vk_initializers.h"
#include <fmt/core.h>
#include <cstring>

DisneyBRDF::DisneyBRDF(bool enableTransmission)
    : _enableTransmission(enableTransmission)
{
}

void DisneyBRDF::build_pipelines(VulkanEngine* engine) {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    if (!vkutil::load_shader_module("../shaders/mesh.vert.spv",
                                     engine->_device, &vertShader)) {
        fmt::println("Error: Failed to load vertex shader for DisneyBRDF");
        return;
    }

    const char* fragPath = _enableTransmission
        ? "../shaders/pbr/disney_bsdf.frag.spv"
        : "../shaders/pbr/disney_brdf.frag.spv";

    if (!vkutil::load_shader_module(fragPath, engine->_device, &fragShader)) {
        fmt::println("Error: Failed to load Disney fragment shader: {}", fragPath);
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

    fmt::println("DisneyBRDF ({}): Pipelines built successfully",
                 _enableTransmission ? "BSDF" : "BRDF");
}

void DisneyBRDF::clear_resources(VkDevice device) {
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

    fmt::println("DisneyBRDF: Resources cleaned up");
}

MaterialInstance DisneyBRDF::write_material(
    VkDevice device,
    MaterialPass pass,
    const BaseMaterialConstants& baseConstants,
    VkBuffer dataBuffer,
    uint32_t dataBufferOffset,
    DescriptorAllocatorGrowable& descriptorAllocator
) {
    DisneyMaterialConstants constants = to_disney_constants(baseConstants);

    return write_disney_material(
        device, pass, constants,
        dataBuffer, dataBufferOffset,
        descriptorAllocator
    );
}

MaterialInstance DisneyBRDF::write_disney_material(
    VkDevice device,
    MaterialPass pass,
    const DisneyMaterialConstants& constants,
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
        sizeof(DisneyMaterialConstants),
        dataBufferOffset,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    );
    _writer.update_set(device, instance.materialSet);

    return instance;
}

DisneyMaterialConstants DisneyBRDF::to_disney_constants(
    const BaseMaterialConstants& base
) {
    DisneyMaterialConstants result = create_default();

    result.baseColor = base.baseColorFactor;
    result.metallic = base.metallicFactor;
    result.roughness = base.roughnessFactor;
    result.normalScale = base.normalScale;
    result.occlusionStrength = base.occlusionStrength;
    result.emissiveColor = base.emissiveFactor;
    result.emissiveStrength = base.emissiveFactor.w;
    result.alphaCutoff = base.alphaCutoff;
    result.alphaMode = base.alphaMode;

    result.baseColorTexID = base.baseColorTexID;
    result.normalTexID = base.normalTexID;
    result.metallicRoughnessTexID = base.metallicRoughnessTexID;
    result.emissiveTexID = base.emissiveTexID;

    if (base.doubleSided) {
        result.flags |= DisneyFlags::DOUBLE_SIDED;
    }

    return result;
}

DisneyMaterialConstants DisneyBRDF::create_default() {
    DisneyMaterialConstants mat{};

    mat.baseColor = glm::vec4(1.0f);
    mat.metallic = 0.0f;
    mat.roughness = 0.5f;
    mat.subsurface = 0.0f;
    mat.specular = 0.5f;
    mat.specularTint = 0.0f;
    mat.anisotropic = 0.0f;
    mat.sheen = 0.0f;
    mat.sheenTint = 0.5f;
    mat.clearcoat = 0.0f;
    mat.clearcoatGloss = 1.0f;
    mat.transmission = 0.0f;
    mat.transmissionRoughness = 0.0f;

    mat.ior = 1.5f;
    mat.anisotropicRotation = 0.0f;
    mat.emissiveStrength = 1.0f;
    mat.normalScale = 1.0f;
    mat.occlusionStrength = 1.0f;
    mat.alphaCutoff = 0.5f;
    mat.alphaMode = 0;
    mat.flags = 0;

    mat.emissiveColor = glm::vec4(0.0f);

    mat.baseColorTexID = UINT32_MAX;
    mat.normalTexID = UINT32_MAX;
    mat.metallicRoughnessTexID = UINT32_MAX;
    mat.emissiveTexID = UINT32_MAX;
    mat.occlusionTexID = UINT32_MAX;
    mat.specularTexID = UINT32_MAX;
    mat.transmissionTexID = UINT32_MAX;
    mat.sheenTexID = UINT32_MAX;
    mat.clearcoatTexID = UINT32_MAX;
    mat.clearcoatRoughnessTexID = UINT32_MAX;
    mat.anisotropyTexID = UINT32_MAX;
    mat.subsurfaceColorTexID = UINT32_MAX;

    mat.subsurfaceColor = glm::vec3(1.0f, 0.2f, 0.1f);
    mat.subsurfaceRadius = 1.0f;

    mat.thinFilmThickness = 0.0f;
    mat.thinFilmIOR = 1.5f;

    return mat;
}

DisneyMaterialConstants DisneyBRDF::create_metal(
    const glm::vec3& color,
    float roughness
) {
    DisneyMaterialConstants mat = create_default();
    mat.baseColor = glm::vec4(color, 1.0f);
    mat.metallic = 1.0f;
    mat.roughness = roughness;
    mat.specular = 1.0f;
    return mat;
}

DisneyMaterialConstants DisneyBRDF::create_glass(
    float ior,
    float roughness,
    const glm::vec3& tint
) {
    DisneyMaterialConstants mat = create_default();
    mat.baseColor = glm::vec4(tint, 1.0f);
    mat.metallic = 0.0f;
    mat.roughness = roughness;
    mat.transmission = 1.0f;
    mat.transmissionRoughness = roughness;
    mat.ior = ior;
    mat.specular = 0.5f;
    return mat;
}

DisneyMaterialConstants DisneyBRDF::create_subsurface(
    const glm::vec3& baseColor,
    const glm::vec3& subsurfaceColor,
    float subsurface
) {
    DisneyMaterialConstants mat = create_default();
    mat.baseColor = glm::vec4(baseColor, 1.0f);
    mat.subsurface = subsurface;
    mat.subsurfaceColor = subsurfaceColor;
    mat.roughness = 0.4f;
    return mat;
}

bool DisneyBRDF::supports_feature(const char* featureName) const {
    if (strcmp(featureName, "metallic") == 0) return true;
    if (strcmp(featureName, "subsurface") == 0) return true;
    if (strcmp(featureName, "specular") == 0) return true;
    if (strcmp(featureName, "specularTint") == 0) return true;
    if (strcmp(featureName, "anisotropic") == 0) return true;
    if (strcmp(featureName, "sheen") == 0) return true;
    if (strcmp(featureName, "clearcoat") == 0) return true;
    if (strcmp(featureName, "transmission") == 0) return _enableTransmission;
    return false;
}