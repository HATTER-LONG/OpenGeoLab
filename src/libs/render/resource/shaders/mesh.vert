#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

layout(std140, binding = 0) uniform FrameBlock {
    mat4 mvp;
    mat4 view;
    vec4 tint;
    vec4 params; // alpha, point size, unused, unused
    vec4 viewport; // width, height, dpr, depth bias
} frame;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec4 vColor;

void main() {
    vec4 clip = frame.mvp * vec4(position, 1.0);
    clip.z -= frame.viewport.w * clip.w;
    gl_Position = clip;
    gl_PointSize = frame.params.y;
    vNormal = normalize(mat3(frame.view) * normal);
    vColor = color;
}
