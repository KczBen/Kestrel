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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

void main() {
    vec3 fragPos = (cameraUBO.view * model * vec4(inPosition, 1.0)).xyz;
    gl_Position = cameraUBO.proj * vec4(fragPos, 1.0);
}
