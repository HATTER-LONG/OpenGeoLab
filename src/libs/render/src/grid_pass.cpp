/**
 * @file grid_pass.cpp
 * @brief Implements the infinite XZ-plane grid render pass.
 */
#include <opengeolab/render/grid_pass.hpp>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/matrix.hpp>

namespace OpenGeoLab::Render {

namespace {

constexpr const char* kGridVertexShader = R"(#version 450 core

uniform mat4 u_viewProj;
uniform mat4 u_invViewProj;

out vec3 v_nearPoint;
out vec3 v_farPoint;

vec3 gridPlane[6] = vec3[](
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

vec3 unprojectPoint(float x, float y, float z) {
    vec4 unprojected = u_invViewProj * vec4(x, y, z, 1.0);
    return unprojected.xyz / unprojected.w;
}

void main() {
    vec3 p = gridPlane[gl_VertexID];
    v_nearPoint = unprojectPoint(p.x, p.y, 0.0);
    v_farPoint  = unprojectPoint(p.x, p.y, 1.0);
    gl_Position = vec4(p, 1.0);
}
)";

constexpr const char* kGridFragmentShader = R"(#version 450 core

in vec3 v_nearPoint;
in vec3 v_farPoint;

uniform mat4 u_viewProj;
uniform float u_near;
uniform float u_far;

layout(location = 0) out vec4 fragColor;

vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);
    vec4 color = vec4(0.3, 0.3, 0.3, 1.0 - min(line, 1.0));

    if (fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    if (fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;

    return color;
}

float computeDepth(vec3 pos) {
    vec4 clipPos = u_viewProj * vec4(pos, 1.0);
    return (clipPos.z / clipPos.w) * 0.5 + 0.5;
}

float computeLinearDepth(vec3 pos) {
    vec4 clipPos = u_viewProj * vec4(pos, 1.0);
    float clipDepth = (clipPos.z / clipPos.w) * 2.0 - 1.0;
    float linearDepth = (2.0 * u_near * u_far) / (u_far + u_near - clipDepth * (u_far - u_near));
    return linearDepth / u_far;
}

void main() {
    float t = -v_nearPoint.y / (v_farPoint.y - v_nearPoint.y);

    if (t < 0.0) discard;

    vec3 fragPos3D = v_nearPoint + t * (v_farPoint - v_nearPoint);

    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0.0, 0.5 - linearDepth);

    fragColor = grid(fragPos3D, 1.0) + grid(fragPos3D, 0.1);
    fragColor.a *= fading;

    if (fragColor.a < 0.01) discard;
}
)";

} // namespace

void GridPass::setup(int /*width*/, int /*height*/) {
    if(initialized_) {
        return;
    }

    if(!shader_.compile(kGridVertexShader, kGridFragmentShader)) {
        return;
    }

    glGenVertexArrays(1, &vao_);
    initialized_ = vao_ != 0U;
}

void GridPass::execute(const RenderContext& ctx) {
    if(!initialized_) {
        return;
    }

    const glm::mat4 view_proj = ctx.projectionMatrix * ctx.viewMatrix;
    const glm::mat4 inverse_view_proj = glm::inverse(view_proj);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    shader_.use();
    shader_.setMat4("u_viewProj", view_proj);
    shader_.setMat4("u_invViewProj", inverse_view_proj);
    shader_.setFloat("u_near", ctx.nearPlane);
    shader_.setFloat("u_far", ctx.farPlane);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

void GridPass::teardown() {
    if(vao_ != 0U) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0U;
    }

    initialized_ = false;
}

} // namespace OpenGeoLab::Render
