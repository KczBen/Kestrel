#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_16bit_storage : require

layout(push_constant) uniform Push { mat4 model; };

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
}
cameraUBO;

layout(set = 1, binding = 0) uniform MaterialUBO {
    vec4 baseColorFactor;
    vec4 emissiveFactor; // .xyz only
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;

    // KHR_texture_transform
    mat4 baseColorTransform;
    mat4 metallicRoughnessTransform;
    mat4 normalTransform;
    mat4 occlusionTransform;
    mat4 emissiveTransform;
}
material;

layout(set = 1, binding = 1) uniform sampler2D albedoSampler;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D normalSampler;
layout(set = 1, binding = 4) uniform sampler2D aoSampler;
layout(set = 1, binding = 5) uniform sampler2D emissionSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

const vec3 lightPosWorld = vec3(10.0, 10.0, 10.0);
const vec3 lightIntensity = vec3(1.5);

// ============================================================
// GGX Normal Distribution Function (Trowbridge-Reitz)
// ============================================================
float ggxD(float a, float NdotH) {
    float a2 = a * a;
    float d = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (3.14159265358979 * d * d);
}

// Optimized closed-form Smith G2 for GGX (height-correlated, folded):
//   G_V = NdotL * sqrt(NdotV^2*(1-a^2) + a^2)
//   G_L = NdotV * sqrt(NdotL^2*(1-a^2) + a^2)
//   G2 = 0.5 / (G_V + G_L)
// This folds the 1/(4*NdotL*NdotV) denominator into G2.
float smithG2(float NdotL, float NdotV, float a) {
    float a2 = a * a;
    float a2v = NdotL * sqrt(NdotV * (1.0 - a2) + a2);
    float a2l = NdotV * sqrt(NdotL * (1.0 - a2) + a2);
    return 0.5 / (a2v + a2l);
}

// ============================================================
// Schlick Fresnel (vector form)
// ============================================================
vec3 schlickF(vec3 F0, float HdotV) {
    float powers5 = (1.0 - HdotV) * (1.0 - HdotV) * (1.0 - HdotV) * (1.0 - HdotV) * (1.0 - HdotV);
    return F0 + (vec3(1.0) - F0) * powers5;
}

// ============================================================
// Burley Diffuse (Disney renormalized, integrates to 1/pi)
// ============================================================
float16_t burleyF_D(float NdotL, float NdotV, float HdotL, float roughness) {
    float16_t rough = float16_t(roughness);
    float16_t NdotL16 = float16_t(NdotL);
    float16_t NdotV16 = float16_t(NdotV);
    float16_t HdotL16 = float16_t(HdotL);

    float16_t F_D90 = float16_t(0.5) + float16_t(2.0) * rough * HdotL16 * HdotL16;
    float16_t one_minus_ndotl = float16_t(1.0) - NdotL16;
    float16_t one_minus_ndotv = float16_t(1.0) - NdotV16;

    float16_t lightTerm = float16_t(1.0) + (F_D90 - float16_t(1.0)) * pow(one_minus_ndotl, float16_t(5.0));
    float16_t viewTerm  = float16_t(1.0) + (F_D90 - float16_t(1.0)) * pow(one_minus_ndotv, float16_t(5.0));

    return lightTerm * viewTerm;
}

// ============================================================
// Full BSDF: Burley diffuse + GGX specular (Cook-Torrance)
// Energy conserving: diffuse *= (1 - F)
// ============================================================
void main() {
    // --- Material inputs ---
    // Clamp texture coordinate bounds and sample
    vec3 albedo = texture(albedoSampler, fragTexCoord).rgb * material.baseColorFactor.rgb;

    // Metallic-roughness texture: R=metallic, G=roughness
    vec2 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord).rg;
    float metallic = metallicRoughness.r * material.metallicFactor;
    float roughness = metallicRoughness.g * material.roughnessFactor;
    roughness = clamp(roughness, 0.04, 1.0);

    // --- Geometry (float32) ---
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(-fragPos);
    vec3 L = normalize(lightPosWorld - fragPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    float LdotH = max(dot(L, H), 0.0);

    // Skip shading if not facing the light
    if (NdotL <= 0.0) {
        outColor = vec4(albedo * 0.0625, 1.0); // ~dark ambient
        return;
    }

    // --- F0: Fresnel reflectance at normal incidence ---
    // Dielectric: F0 = 0.04; Metal: F0 = baseColor
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Specular: GGX Cook-Torrance ---
    float a = roughness * roughness;  // perceptual roughness squared -> alpha
    a = max(a, 0.0001);               // clamp to avoid singularity

    float D = ggxD(a, NdotH);
    vec3 F = schlickF(F0, HdotV);
    float G2 = smithG2(NdotL, NdotV, a);  // folded G2 (absorbs 1/(4*NdotL*NdotV))

    // f_specular = D * F * G2  (denominator already folded)
    // Multiply by light intensity and NdotL for illumination
    vec3 specular = D * F * G2 * NdotL * lightIntensity;

    // --- Diffuse: Burley ---
    float16_t diffuseScatter = burleyF_D(NdotL, NdotV, LdotH, roughness);

    // f_diffuse = (baseColor / pi) * F_D(l) * F_D(v) * (1 - F)
    vec3 diffuse = float(diffuseScatter) * (vec3(1.0) - F) * (albedo * float16_t(0.31830988618)) * NdotL;

    // Scale by light
    diffuse = diffuse * float(lightIntensity);

    // --- Composite ---
    vec3 radiance = specular + diffuse;

    // Gamma correction (linear -> sRGB)
    radiance = pow(radiance, vec3(1.0 / 2.2));

    outColor = vec4(radiance, 1.0);
}
