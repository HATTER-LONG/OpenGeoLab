#version 440

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform FrameBlock {
    mat4 mvp;
    mat4 view;
    vec4 tint;
    vec4 params;
    vec4 viewport;
} frame;

void main() {
    vec3 rgb = mix(vColor.rgb, frame.tint.rgb, frame.tint.a);
    float alpha = vColor.a * frame.params.x;
    fragColor = vec4(rgb * alpha, alpha);
}
