/**
 * @file opaque_pass.cpp
 * @brief OpaquePass implementation — opaque surface rendering for geometry and mesh.
 */

#include "opaque_pass.hpp"

#include "../core/gpu_buffer.hpp"
#include "draw_batch_utils.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {
namespace {
const char* surface_vertex_shader = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
out vec3 v_worldPos;
out vec3 v_normal;
out vec4 v_color;
void main() {
    v_worldPos = a_position;
    v_normal = a_normal;
    v_color = a_color;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";
const char* surface_fragment_shader = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in vec4 v_color;
uniform vec3 u_cameraPos;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_cameraPos - v_worldPos);
    float ambient = 0.35;
    float headlamp = abs(dot(N, V));
    float skyLight = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15;
    float groundBounce = max(dot(N, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    float lighting = ambient + headlamp * 0.55 + skyLight + groundBounce;
    vec3 litColor = v_color.rgb * min(lighting, 1.0);
    // Premultiply alpha for correct Qt Quick scene-graph compositing
    fragColor = vec4(litColor * u_alpha, u_alpha);
}
)";

[[nodiscard]] constexpr bool hasMode(RenderDisplayModeMask value, RenderDisplayModeMask flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}
} // namespace

bool OpaquePass::onInitialize() {
    if(!m_surfaceShader.compile(surface_vertex_shader, surface_fragment_shader)) {
        LOG_ERROR("OpaquePass: Failed to compile surface shader");
        return false;
    }
    LOG_DEBUG("OpaquePass: Initialized");
    return true;
}

void OpaquePass::onCleanup() {
    m_surfaceShader.release();
    LOG_DEBUG("OpaquePass: Cleaned up");
}

void OpaquePass::render(RenderPassContext& ctx) {
    if(ctx.m_mesh.m_displayMode == RenderDisplayModeMask::Mesh) {
        return;
    }

    auto& geo_buf = ctx.m_geometry.m_buffer;
    const auto& tri_ranges = ctx.m_geometry.m_triangleRanges;

    auto& mesh_buf = ctx.m_mesh.m_buffer;
    const auto& mesh_tri_ranges = ctx.m_mesh.m_triangleRanges;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLContext* ctx_gl = QOpenGLContext::currentContext();
    f->glEnable(GL_DEPTH_TEST);

    float alpha = 1.0f;
    if(ctx.m_params.m_xRayMode) {
        alpha = 0.25f;
        // Enable blending for transparency
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        f->glDepthMask(GL_FALSE);
    }
    m_surfaceShader.bind();
    m_surfaceShader.setUniformMatrix4("u_viewMatrix", ctx.m_params.m_viewMatrix);
    m_surfaceShader.setUniformMatrix4("u_projMatrix", ctx.m_params.m_projMatrix);
    m_surfaceShader.setUniformVec3("u_cameraPos", ctx.m_params.m_cameraPos);
    m_surfaceShader.setUniformFloat("u_alpha", alpha);
    if(ctx.m_geometry.hasGeometry()) {
        geo_buf.bindForDraw();
        // Push surfaces slightly back in depth so coplanar wireframe edges
        // on visible faces pass the depth test
        f->glEnable(GL_POLYGON_OFFSET_FILL);
        f->glPolygonOffset(1.0f, 1.0f);
        std::vector<GLsizei> counts;
        std::vector<const void*> offsets;
        PassUtil::buildIndexedBatch(
            tri_ranges, [](const DrawRange&) { return true; }, counts, offsets);
        PassUtil::multiDrawElements(ctx_gl, f, GL_TRIANGLES, counts, offsets);

        f->glDisable(GL_POLYGON_OFFSET_FILL);
        geo_buf.unbind();
    }
    if(ctx.m_mesh.hasMesh()) {
        mesh_buf.bindForDraw();
        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            mesh_tri_ranges, [](const DrawRange&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx_gl, f, GL_TRIANGLES, firsts, counts);
        mesh_buf.unbind();
    }

    if(ctx.m_params.m_xRayMode) {
        f->glDisable(GL_BLEND);
        f->glDepthMask(GL_TRUE);
    }
    m_surfaceShader.release();
}
} // namespace OpenGeoLab::Render