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
layout(set = 1, binding = 1) uniform sampler2D packedNormalSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

const float roughness = 0.3;
const vec3 lightPosWorld = vec3(10.0, 10.0, 10.0);

void main() {
    // Interpolated vertex colours for now
    vec3 albedo = fragColor;
    // vec3 albedo = texture(albedoSampler, fragTexCoord);

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(-fragPos); // camera at origin in view space
    vec3 lightPosView = (cameraUBO.view * vec4(lightPosWorld, 1.0)).xyz;
    vec3 L = normalize(lightPosView - fragPos);

    float dotNV = dot(N, V);
    float dotNL = dot(N, L);

    float thetaIncidence = acos(dotNL);
    float thetaReflection = acos(dotNV);

    vec3 lTan = normalize(L - dotNL * N);
    vec3 vTan = normalize(V - dotNV * N);

    outColor = texture(albedoSampler, fragTexCoord);
}
