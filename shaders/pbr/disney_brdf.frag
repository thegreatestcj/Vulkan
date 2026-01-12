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

layout(set = 1, binding = 0) uniform DisneyMaterial {
    vec4 baseColor;

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
    uint alphaMode;
    uint flags;

    vec4 emissiveColor;

    uint baseColorTexID;
    uint normalTexID;
    uint metallicRoughnessTexID;
    uint emissiveTexID;

    uint occlusionTexID;
    uint specularTexID;
    uint transmissionTexID;
    uint sheenTexID;

    uint clearcoatTexID;
    uint clearcoatRoughnessTexID;
    uint anisotropyTexID;
    uint subsurfaceColorTexID;

    vec3 subsurfaceColor;
    float subsurfaceRadius;
} mat;

const float PI = 3.14159265359;
const float INV_PI = 0.31830988618;
const float EPSILON = 1e-6;
const uint INVALID_TEX = 0xFFFFFFFF;
const uint FLAG_DOUBLE_SIDED = 1u;

float sqr(float x) { return x * x; }

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec4 sampleTex(uint id, vec2 uv, vec4 def) {
    return id == INVALID_TEX ? def : texture(allTextures[id], uv);
}

float Fd_Burley(float NdotV, float NdotL, float LdotH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float lightScatter = 1.0 + (f90 - 1.0) * pow(1.0 - NdotL, 5.0);
    float viewScatter = 1.0 + (f90 - 1.0) * pow(1.0 - NdotV, 5.0);
    return lightScatter * viewScatter * INV_PI;
}

float Fd_Subsurface(float NdotV, float NdotL, float LdotH, float roughness) {
    float Fss90 = roughness * LdotH * LdotH;
    float Fss = (1.0 + (Fss90 - 1.0) * pow(1.0 - NdotL, 5.0))
    * (1.0 + (Fss90 - 1.0) * pow(1.0 - NdotV, 5.0));
    return 1.25 * INV_PI * (Fss * (1.0 / (NdotL + NdotV + EPSILON) - 0.5) + 0.5);
}

float D_GTR2(float NdotH, float alpha) {
    float a2 = alpha * alpha;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float D_GTR1(float NdotH, float alpha) {
    if (alpha >= 1.0) return INV_PI;
    float a2 = alpha * alpha;
    float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

float G1_Smith_GGX(float NdotV, float alpha) {
    float a2 = alpha * alpha;
    float NdotV2 = NdotV * NdotV;
    return 2.0 * NdotV / (NdotV + sqrt(a2 + (1.0 - a2) * NdotV2));
}

vec3 F_Schlick(float VdotH, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

float F_Schlick_Scalar(float VdotH, float F0) {
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

vec3 evaluateSheen(vec3 baseColor, float LdotH) {
    float Csheen = pow(1.0 - LdotH, 5.0);
    float lum = luminance(baseColor);
    vec3 Ctint = lum > 0.0 ? baseColor / lum : vec3(1.0);
    vec3 CsheenColor = mix(vec3(1.0), Ctint, mat.sheenTint);
    return Csheen * mat.sheen * CsheenColor;
}

float evaluateClearcoat(float NdotH, float NdotL, float NdotV, float LdotH) {
    float gloss = mix(0.1, 0.001, mat.clearcoatGloss);
    float D = D_GTR1(NdotH, gloss);
    float F = F_Schlick_Scalar(LdotH, 0.04);
    float G = G1_Smith_GGX(NdotL, 0.25) * G1_Smith_GGX(NdotV, 0.25);
    return mat.clearcoat * D * F * G;
}

void main() {
    vec4 baseColorSample = sampleTex(mat.baseColorTexID, inUV, vec4(1.0));
    vec3 baseColor = baseColorSample.rgb * mat.baseColor.rgb * inColor;
    float alpha = baseColorSample.a * mat.baseColor.a;

    if (mat.alphaMode == 1u && alpha < mat.alphaCutoff) {
        discard;
    }

    vec4 mrSample = sampleTex(mat.metallicRoughnessTexID, inUV, vec4(1.0, 1.0, 0.0, 0.0));
    float metallic = mrSample.b * mat.metallic;
    float roughness = max(mrSample.g * mat.roughness, 0.045);
    float roughness2 = roughness * roughness;

    vec3 N = normalize(inNormal);
    if ((mat.flags & FLAG_DOUBLE_SIDED) != 0u && !gl_FrontFacing) {
        N = -N;
    }

    vec3 L = normalize(-scene.sunlightDirection.xyz);
    vec3 V = vec3(0.0, 0.0, 1.0);
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.001);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 Lo = vec3(0.0);

    if (NdotL > 0.0) {
        float dielectricF0 = 0.08 * mat.specular;
        float lum = luminance(baseColor);
        vec3 Ctint = lum > 0.0 ? baseColor / lum : vec3(1.0);
        vec3 Cspec0 = mix(dielectricF0 * mix(vec3(1.0), Ctint, mat.specularTint),
        baseColor, metallic);

        float D = D_GTR2(NdotH, roughness2);
        float G = G1_Smith_GGX(NdotL, roughness2) * G1_Smith_GGX(NdotV, roughness2);
        vec3 F = F_Schlick(LdotH, Cspec0);

        vec3 specular = D * G * F / (4.0 * NdotL * NdotV + EPSILON);

        float Fd = mix(
        Fd_Burley(NdotV, NdotL, LdotH, roughness),
        Fd_Subsurface(NdotV, NdotL, LdotH, roughness),
        mat.subsurface
        );

        vec3 diffuseColor = mix(baseColor, mat.subsurfaceColor, mat.subsurface);
        vec3 diffuse = diffuseColor * (1.0 - metallic) * Fd;

        vec3 sheenContrib = vec3(0.0);
        if (mat.sheen > 0.0) {
            sheenContrib = evaluateSheen(baseColor, LdotH);
        }

        float clearcoatContrib = 0.0;
        if (mat.clearcoat > 0.0) {
            clearcoatContrib = evaluateClearcoat(NdotH, NdotL, NdotV, LdotH);
        }

        vec3 kD = (1.0 - F) * (1.0 - metallic);
        vec3 brdf = kD * diffuse + specular + sheenContrib * (1.0 - metallic);

        brdf = brdf * (1.0 - mat.clearcoat * F_Schlick_Scalar(NdotV, 0.04))
        + vec3(clearcoatContrib);

        vec3 radiance = scene.sunlightColor.rgb * scene.sunlightDirection.w;
        Lo += brdf * radiance * NdotL;
    }

    vec3 ambient = scene.ambientColor.rgb * baseColor * (1.0 - metallic) * INV_PI * 0.3;

    vec3 F0_env = mix(vec3(0.04), baseColor, metallic);
    vec3 F_env = F0_env + (max(vec3(1.0 - roughness), F0_env) - F0_env)
    * pow(1.0 - NdotV, 5.0);
    ambient += F_env * scene.ambientColor.rgb * 0.2;

    float ao = sampleTex(mat.occlusionTexID, inUV, vec4(1.0)).r;
    ao = mix(1.0, ao, mat.occlusionStrength);

    Lo += ambient * ao;

    vec3 emissive = sampleTex(mat.emissiveTexID, inUV, vec4(0.0)).rgb;
    emissive *= mat.emissiveColor.rgb * mat.emissiveStrength;
    Lo += emissive;

    outColor = vec4(Lo, alpha);
}