#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_16bit_storage : require

layout(push_constant) uniform Push {
    mat4 model;
    mat4 normalMatrix;
};

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
}
camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord0;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec2 inTexCoord1;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord[2];
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragPos;

void main() {
    fragPos = (camera.view * model * vec4(inPosition, 1.0)).xyz;
    gl_Position = camera.proj * vec4(fragPos, 1.0);
    fragColor = inColor;
    fragTexCoord[0] = inTexCoord0;
    fragTexCoord[1] = inTexCoord1;
    fragNormal = mat3(normalMatrix) * inNormal;
}
