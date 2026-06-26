#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_16bit_storage : require

layout(push_constant) uniform Push {
    mat4 model;
    mat4 normal;
};

struct light {
    vec3 position;
    float intensity;
    vec3 colour;
    float maxRange;
};

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
}
camera;

layout(set = 0, binding = 1) uniform LightUBO {
    light lights[16];
    int lightCount;
}
lights;

layout(set = 1, binding = 0) uniform MaterialUBO {
    vec4 baseColorFactor;
    vec4 emissiveFactor; // .xyz only
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;

    // UV indices for each texture
    int baseColorUVIndex;
    int metallicRoughnessUVIndex;
    int normalUVIndex;
    int aoUVIndex;
    int emissiveUVIndex;

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
layout(location = 1) in vec2 fragTexCoord[2];
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

// same shader as the sky, so we know what to reflect
vec3 GetProceduralSky(vec3 dir) {
    vec3 horizonColor = vec3(0.6, 0.8, 1.0);
    vec3 zenithColor = vec3(0.1, 0.3, 0.8);
    vec3 groundColor = vec3(0.05, 0.04, 0.03);

    float skyT = clamp(dir.y, 0.0, 1.0);
    vec3 skyGradient = mix(horizonColor, zenithColor, skyT);

    float groundT = smoothstep(-0.2, 0.1, dir.y);
    return mix(groundColor, skyGradient, groundT);
}

vec3 getNormalFromMap(vec2 uv) {
    vec3 tangentNormal = texture(normalSampler, uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= material.normalScale;

    vec3 Q1 = dFdx(fragPos);
    vec3 Q2 = dFdy(fragPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N = normalize(fragNormal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// Basic Blinn-Phong stuff
void main() {
    vec2 uvAlbedo = (material.baseColorTransform * vec4(fragTexCoord[material.baseColorUVIndex], 0.0, 1.0)).xy;
    vec2 uvMR =
        (material.metallicRoughnessTransform * vec4(fragTexCoord[material.metallicRoughnessUVIndex], 0.0, 1.0)).xy;
    vec2 uvNorm = (material.normalTransform * vec4(fragTexCoord[material.normalUVIndex], 0.0, 1.0)).xy;
    vec2 uvAO = (material.occlusionTransform * vec4(fragTexCoord[material.aoUVIndex], 0.0, 1.0)).xy;
    vec2 uvEmissive = (material.emissiveTransform * vec4(fragTexCoord[material.emissiveUVIndex], 0.0, 1.0)).xy;

    vec3 albedo = texture(albedoSampler, uvAlbedo).rgb * material.baseColorFactor.rgb;
    vec4 mrSample = texture(metallicRoughnessSampler, uvMR);
    float roughness = mrSample.g * material.roughnessFactor;
    float metallic = mrSample.b * material.metallicFactor;
    float ao = mix(1.0, texture(aoSampler, uvAO).r, material.occlusionStrength);
    vec3 emission = texture(emissionSampler, uvEmissive).rgb * material.emissiveFactor.xyz;

    vec3 N = getNormalFromMap(uvNorm);
    vec3 camPos = camera.invView[3].xyz;
    vec3 V = normalize(camPos - fragPos);

    // Initialize accumulators for all scene lights
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    // Calculate light-independent Blinn-Phong factors once outside the loop
    float shininess = exp2(8.0 * (1.0 - roughness));
    vec3 specColor = mix(vec3(1.0), albedo, metallic);

    // 1. Accumulate analytical lighting from scene lights
    for (int i = 0; i < lights.lightCount; ++i) {
        light l = lights.lights[i];

        // Calculate vector from fragment position to the light source
        vec3 toLight = l.position - fragPos;
        float dist = length(toLight);

        // Early out: if the light is too far away, it contributes absolutely nothing
        if (dist > l.maxRange) {
            continue;
        }

        // Normalize direction vector for lighting math
        vec3 lightDir = normalize(toLight);
        vec3 H = normalize(lightDir + V);

        // Linear attenuation formula: smoothly fades to 0.0 at maxRange, scaled by intensity
        float attenuation = clamp(1.0 - (dist / l.maxRange), 0.0, 1.0) * l.intensity;
        vec3 dynamicLightColor = l.colour * attenuation;

        // Diffuse
        float NdotL = max(dot(N, lightDir), 0.0);
        totalDiffuse += albedo * NdotL * dynamicLightColor * (1.0 - metallic);

        // Specular
        float specFactor = pow(max(dot(N, H), 0.0), shininess);
        totalSpecular += specColor * specFactor * dynamicLightColor;
    }

    // 2. Sky Ambient / Fake GI (Calculated once globally, NOT per-light)
    vec3 R = reflect(-V, N);
    vec3 blurredR = normalize(mix(R, N, roughness));

    vec3 ambientDiffuse = GetProceduralSky(N) * albedo * (1.0 - metallic);
    vec3 ambientSpecular = GetProceduralSky(blurredR) * specColor * (1.0 - roughness);
    vec3 ambient = (ambientDiffuse + ambientSpecular) * ao * 0.5;

    // 3. Final Combine
    vec3 color = totalDiffuse + totalSpecular + ambient + emission;

    outColor = vec4(color, 1.0);
}
