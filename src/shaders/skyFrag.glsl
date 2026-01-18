#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_16bit_storage : require

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
}
cameraUBO;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 ndc = fragUV * 2.0 - 1.0;

    vec4 clipSpace = vec4(ndc, 1.0, 1.0);

    vec4 viewSpace = cameraUBO.invProj * clipSpace;
    viewSpace /= viewSpace.w;

    vec3 worldDir = normalize((cameraUBO.invView * vec4(viewSpace.xyz, 0.0)).xyz);

    float t = worldDir.y * 0.5 + 0.5;

    // Sky gradient colors
    vec3 horizonColor = vec3(0.6, 0.8, 1.0);
    vec3 zenithColor = vec3(0.1, 0.3, 0.8);

    vec3 skyColor = mix(horizonColor, zenithColor, t);
    fragColor = vec4(skyColor, 1.0);
}