#version 440

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vLocal;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform FrameBlock {
    mat4 mvp;
    mat4 view;
    vec4 tint;
    vec4 params;
    vec4 viewport;
} frame;

void main() {
    float radius = length(vLocal);
    float antialias = max(fwidth(radius) * 1.5, 0.02);
    float coverage = 1.0 - smoothstep(1.0 - antialias, 1.0, radius);
    if (coverage <= 0.0)
        discard;

    vec3 base = mix(vColor.rgb, frame.tint.rgb, frame.tint.a);
    float rim = smoothstep(0.62 - antialias, 0.62 + antialias, radius);
    vec3 color = mix(mix(base, vec3(1.0), 0.28), base * 0.28, rim);
    float alpha = vColor.a * frame.params.x * coverage;
    fragColor = vec4(color * alpha, alpha);
}
