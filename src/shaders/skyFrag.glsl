#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_16bit_storage : require

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

void main() { fragColor = vec4(fragUV, 1.0, 1.0); }