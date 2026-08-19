#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

layout(std140, binding = 0) uniform FrameBlock {
    mat4 mvp;
    mat4 view;
    vec4 tint;
    vec4 params;
    vec4 viewport;
} frame;

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vLocal;

void main() {
    const vec2 corners[6] = vec2[6](
        vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2(-1.0,  1.0),
        vec2(-1.0,  1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0));
    vec2 corner = corners[gl_VertexIndex];
    vec4 clip = frame.mvp * vec4(position, 1.0);
    vec2 pixelToNdc = vec2(1.0 / frame.viewport.x, 1.0 / frame.viewport.y);
    clip.xy += corner * frame.params.y * pixelToNdc * clip.w;
    clip.z -= frame.viewport.w * clip.w;
    gl_Position = clip;
    vColor = color;
    vLocal = corner;
}
