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

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D aoSampler;
layout(set = 1, binding = 4) uniform sampler2D emissionSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

const float16_t roughness = float16_t(0.3);
const vec3 lightPosWorld = vec3(10.0, 10.0, 10.0);
const vec3 lightIntensity = vec3(1.5); // Hardcoded light color/brightness
const float16_t INV_PI = float16_t(0.31830988618);

float16_t DisneysDiffuse(float16_t NdotL, float16_t NdotV, float16_t LdotH, float16_t rough) {
    float16_t FD90 = float16_t(0.5) + float16_t(2.0) * LdotH * LdotH * rough;

    float16_t lightScatter = float16_t(1.0) + (FD90 - float16_t(1.0)) * pow(float16_t(1.0) - NdotL, float16_t(5.0));
    float16_t viewScatter = float16_t(1.0) + (FD90 - float16_t(1.0)) * pow(float16_t(1.0) - NdotV, float16_t(5.0));

    return lightScatter * viewScatter;
}

void main() {
    vec3 albedo = vec3(fragTexCoord, 1.0);

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(-fragPos);
    vec3 lightPosView = (cameraUBO.view * vec4(lightPosWorld, 1.0)).xyz;
    vec3 L = normalize(lightPosView - fragPos);
    vec3 H = normalize(L + V);

    float16_t NdotL = float16_t(max(dot(N, L), 0.0));
    float16_t NdotV = float16_t(max(dot(N, V), 0.0));
    float16_t LdotH = float16_t(max(dot(L, H), 0.0));

    float16_t diffuseScatter = DisneysDiffuse(NdotL, NdotV, LdotH, roughness);

    vec3 diffuseEvaluation = (albedo * float(INV_PI)) * float(diffuseScatter * NdotL);

    vec3 finalColor = diffuseEvaluation * lightIntensity;

    // Simple gamma correction (approximate linear to sRGB)
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(albedo, 1.0);
}