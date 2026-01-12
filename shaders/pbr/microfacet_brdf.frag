#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} scene;

layout(set = 0, binding = 1) uniform sampler2D allTextures[];

layout(set = 1, binding = 0) uniform MicrofacetMaterial {
    vec4 baseColorFactor;
    vec4 emissiveFactor;

    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;

    uint baseColorTexID;
    uint normalTexID;
    uint metallicRoughnessTexID;
    uint emissiveTexID;

    float alphaCutoff;
    uint alphaMode;
    uint doubleSided;
    uint _pad0;

    float anisotropy;
    float anisotropyRotation;
    float ior;
    float specularTint;

    float clearcoat;
    float clearcoatRoughness;
    float sheen;
    float sheenTint;

    uint anisotropyTexID;
    uint clearcoatTexID;
    uint clearcoatRoughnessTexID;
    uint sheenTexID;

    uint occlusionTexID;
} mat;

const float PI = 3.14159265359;
const float INV_PI = 0.31830988618;
const float EPSILON = 1e-6;
const uint INVALID_TEX = 0xFFFFFFFF;

vec4 sampleTex(uint id, vec2 uv, vec4 def) {
    return id == INVALID_TEX ? def : texture(allTextures[id], uv);
}

vec3 computeF0(vec3 baseColor, float metallic, float ior) {
    float dielectricF0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
    return mix(vec3(dielectricF0), baseColor, metallic);
}

float D_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + EPSILON);
}

float G2_Smith_HeightCorrelated(float NdotV, float NdotL, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float lambdaV = NdotL * sqrt(a2 + (1.0 - a2) * NdotV * NdotV);
    float lambdaL = NdotV * sqrt(a2 + (1.0 - a2) * NdotL * NdotL);
    return 2.0 * NdotL * NdotV / (lambdaV + lambdaL + EPSILON);
}

vec3 F_Schlick(float VdotH, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

vec3 diffuse_Burley(vec3 baseColor, float roughness, float NdotV, float NdotL, float VdotH) {
    float FD90 = 0.5 + 2.0 * VdotH * VdotH * roughness;
    float lightScatter = 1.0 + (FD90 - 1.0) * pow(1.0 - NdotL, 5.0);
    float viewScatter = 1.0 + (FD90 - 1.0) * pow(1.0 - NdotV, 5.0);
    return baseColor * INV_PI * lightScatter * viewScatter;
}

float clearcoatSpecular(float NdotH, float NdotV, float NdotL, float VdotH) {
    float ccRoughness = mix(0.089, 0.6, mat.clearcoatRoughness);
    float D = D_GGX(NdotH, ccRoughness);
    float G = G2_Smith_HeightCorrelated(NdotV, NdotL, ccRoughness);
    float F = 0.04 + 0.96 * pow(1.0 - VdotH, 5.0);
    return D * G * F;
}

void main() {
    vec4 baseColorSample = sampleTex(mat.baseColorTexID, inUV, vec4(1.0));
    vec3 baseColor = baseColorSample.rgb * mat.baseColorFactor.rgb * inColor;
    float alpha = baseColorSample.a * mat.baseColorFactor.a;

    if (mat.alphaMode == 1 && alpha < mat.alphaCutoff) {
        discard;
    }

    vec4 mrSample = sampleTex(mat.metallicRoughnessTexID, inUV, vec4(1.0, 1.0, 0.0, 0.0));
    float metallic = mrSample.b * mat.metallicFactor;
    float roughness = max(mrSample.g * mat.roughnessFactor, 0.04);

    vec3 N = normalize(inNormal);
    if (mat.doubleSided == 1 && !gl_FrontFacing) {
        N = -N;
    }

    vec3 L = normalize(-scene.sunlightDirection.xyz);
    vec3 V = vec3(0.0, 0.0, 1.0);
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.001);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 F0 = computeF0(baseColor, metallic, mat.ior);

    vec3 Lo = vec3(0.0);

    if (NdotL > 0.0) {
        float D = D_GGX(NdotH, roughness);
        float G = G2_Smith_HeightCorrelated(NdotV, NdotL, roughness);
        vec3 F = F_Schlick(VdotH, F0);

        vec3 specular = D * G * F / (4.0 * NdotV * NdotL + EPSILON);

        vec3 diffuseColor = baseColor * (1.0 - metallic);
        vec3 diffuse = diffuse_Burley(diffuseColor, roughness, NdotV, NdotL, VdotH);

        vec3 kD = (1.0 - F) * (1.0 - metallic);
        vec3 brdf = kD * diffuse + specular;

        if (mat.clearcoat > 0.0) {
            float Fc = 0.04 + 0.96 * pow(1.0 - VdotH, 5.0);
            float ccSpec = clearcoatSpecular(NdotH, NdotV, NdotL, VdotH);
            brdf *= (1.0 - mat.clearcoat * Fc);
            brdf += mat.clearcoat * ccSpec * vec3(1.0);
        }

        vec3 radiance = scene.sunlightColor.rgb * scene.sunlightDirection.w;
        Lo += brdf * radiance * NdotL;
    }

    vec3 ambient = scene.ambientColor.rgb * baseColor * (1.0 - metallic) * 0.3;
    Lo += ambient;

    vec3 emissive = sampleTex(mat.emissiveTexID, inUV, vec4(0.0)).rgb;
    emissive *= mat.emissiveFactor.rgb;
    Lo += emissive;

    outColor = vec4(Lo, alpha);
}