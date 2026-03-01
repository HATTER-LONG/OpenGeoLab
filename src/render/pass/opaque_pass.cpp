/**
 * @file opaque_pass.cpp
 * @brief OpaquePass implementation — opaque surface rendering for geometry and mesh.
 */

#include "opaque_pass.hpp"

#include "draw_batch_utils.hpp"
#include "render/core/gpu_buffer.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {

const char* SURFACE_VERTEX_SHADER = R"(
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

const char* SURFACE_FRAGMENT_SHADER = R"(
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

} // anonymous namespace

bool OpaquePass::onInitialize() {
    if(!m_surfaceShader.compile(SURFACE_VERTEX_SHADER, SURFACE_FRAGMENT_SHADER)) {
        LOG_ERROR("OpaquePass: Failed to compile surface shader");
        return false;
    }

    LOG_DEBUG("OpaquePass: Initialized");
    return true;
}

void OpaquePass::onCleanup() { LOG_DEBUG("OpaquePass: Cleaned up"); }

void OpaquePass::render(const PassRenderParams& params,
                        GpuBuffer& geomBuffer,
                        const std::vector<DrawRangeEx>& triangleRanges,
                        GpuBuffer& meshBuffer,
                        uint32_t meshSurfaceCount,
                        RenderDisplayModeMask meshDisplayMode) {
    if(!isInitialized() || params.xRayMode) {
        return; // TransparentPass handles X-ray mode
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    f->glEnable(GL_DEPTH_TEST);

    // Push surfaces slightly back in depth so coplanar wireframe edges
    // on visible faces pass the depth test
    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(1.0f, 1.0f);

    m_surfaceShader.bind();
    m_surfaceShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
    m_surfaceShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
    m_surfaceShader.setUniformVec3("u_cameraPos", params.cameraPos);
    m_surfaceShader.setUniformFloat("u_alpha", 1.0f);

    // --- Geometry triangles (indexed drawing) ---
    if(!triangleRanges.empty() && geomBuffer.vertexCount() > 0) {
        geomBuffer.bindForDraw();

        std::vector<GLsizei> counts;
        std::vector<const void*> offsets;
        PassUtil::buildIndexedBatch(
            triangleRanges, [](const DrawRangeEx&) { return true; }, counts, offsets);
        PassUtil::multiDrawElements(ctx, f, GL_TRIANGLES, counts, offsets);

        geomBuffer.unbind();
    }

    // --- Mesh surface triangles (array drawing) ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Surface) && meshSurfaceCount > 0 &&
       meshBuffer.vertexCount() > 0) {
        meshBuffer.bindForDraw();

        f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(meshSurfaceCount));

        meshBuffer.unbind();
    }

    m_surfaceShader.release();
    f->glDisable(GL_POLYGON_OFFSET_FILL);
}

} // namespace OpenGeoLab::Render
