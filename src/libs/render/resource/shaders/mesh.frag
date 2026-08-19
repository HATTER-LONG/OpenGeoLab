#version 440

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform FrameBlock {
    mat4 mvp;
    mat4 view;
    vec4 tint;
    vec4 params;
    vec4 viewport;
} frame;

void main() {
    vec3 n = normalize(vNormal);
    float lighting = 0.35
        + abs(dot(n, vec3(0.0, 0.0, 1.0))) * 0.55
        + max(dot(n, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15
        + max(dot(n, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    vec3 lit = vColor.rgb * min(lighting, 1.0);
    vec3 rgb = mix(lit, frame.tint.rgb, frame.tint.a);
    float alpha = vColor.a * frame.params.x;
    fragColor = vec4(rgb * alpha, alpha);
}
