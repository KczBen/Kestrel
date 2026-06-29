#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
#extension GL_EXT_shader_16bit_storage : require

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
}
camera;

layout(location = 0) in vec2 fragNDC;
layout(location = 0) out vec4 outColor;

// Grid colours
const vec3 COL_THIN = vec3(0.25);               // 1 m lines
const vec3 COL_THICK = vec3(0.40);              // 10 m lines
const vec3 COL_X_AXIS = vec3(0.80, 0.18, 0.18); // X axis (red)
const vec3 COL_Z_AXIS = vec3(0.18, 0.18, 0.80); // Z axis (blue)

const float ALPHA_THIN = 0.55;
const float ALPHA_THICK = 0.90;

const float FADE_START = 0.55;
const float FADE_END = 0.92;

const float AXIS_PX = 1.5;

vec3 ndcToWorld(vec3 ndc) {
    vec4 vs = camera.invProj * vec4(ndc, 1.0);
    vs /= vs.w;
    return (camera.invView * vs).xyz;
}

float gridAlpha(vec2 worldXZ, float cellM) {
    vec2 coord = worldXZ / cellM;
    vec2 d = fwidth(coord); // world units per pixel
    vec2 lines = abs(fract(coord - 0.5) - 0.5) / d;
    return 1.0 - min(min(lines.x, lines.y), 1.0);
}

void main() {

    vec3 nearW = ndcToWorld(vec3(fragNDC, 0.0));
    vec3 farW = ndcToWorld(vec3(fragNDC, 1.0));

    float denom = farW.y - nearW.y;
    if (abs(denom) < 1e-6)
        discard;

    float t = -nearW.y / denom;
    if (t <= 0.0)
        discard;

    vec3 hit = nearW + t * (farW - nearW);

    float fade = 1.0 - smoothstep(FADE_START, FADE_END, t);
    if (fade < 0.001)
        discard;

    vec2 worldDeriv = fwidth(hit.xz);
    float fineDensity = max(worldDeriv.x, worldDeriv.y);
    float fineFade = 1.0 - smoothstep(0.4, 1.2, fineDensity);

    float a1 = gridAlpha(hit.xz, 1.0) * ALPHA_THIN * fineFade;
    float a10 = gridAlpha(hit.xz, 10.0) * ALPHA_THICK;

    float alpha = max(a1, a10);
    vec3 color = mix(COL_THIN, COL_THICK, smoothstep(0.0, 1.0, a10));

    if (abs(hit.z) < AXIS_PX * worldDeriv.y) {
        color = COL_X_AXIS;
        alpha = max(alpha, 0.95);
    }

    if (abs(hit.x) < AXIS_PX * worldDeriv.x) {
        color = COL_Z_AXIS;
        alpha = max(alpha, 0.95);
    }

    alpha *= fade;
    if (alpha < 0.001)
        discard;

    outColor = vec4(color, alpha);

    // depth writes are disabled in the material so this isn't used here but it could be
    vec4 clipPos = camera.proj * camera.view * vec4(hit, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;
}