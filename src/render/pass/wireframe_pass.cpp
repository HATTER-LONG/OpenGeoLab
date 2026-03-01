/**
 * @file wireframe_pass.cpp
 * @brief WireframePass implementation — edges and points rendering.
 */

#include "wireframe_pass.hpp"

#include "draw_batch_utils.hpp"
#include "render/core/gpu_buffer.hpp"
#include "util/color_map.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {

const char* FLAT_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_PointSize = u_pointSize;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* FLAT_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 v_color;
out vec4 fragColor;
void main() {
    fragColor = vec4(v_color.rgb, v_color.a);
}
)";

[[nodiscard]] constexpr bool hasMode(RenderDisplayModeMask value, RenderDisplayModeMask flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

} // anonymous namespace

bool WireframePass::onInitialize() {
    if(!m_flatShader.compile(FLAT_VERTEX_SHADER, FLAT_FRAGMENT_SHADER)) {
        LOG_ERROR("WireframePass: Failed to compile flat shader");
        return false;
    }

    LOG_DEBUG("WireframePass: Initialized");
    return true;
}

void WireframePass::onCleanup() { LOG_DEBUG("WireframePass: Cleaned up"); }

void WireframePass::render(const PassRenderParams& params,
                           GpuBuffer& geomBuffer,
                           const std::vector<DrawRangeEx>& lineRanges,
                           const std::vector<DrawRangeEx>& pointRanges,
                           GpuBuffer& meshBuffer,
                           uint32_t meshSurfaceCount,
                           uint32_t meshWireframeCount,
                           uint32_t meshNodeCount,
                           RenderDisplayModeMask meshDisplayMode) {
    if(!isInitialized()) {
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    f->glEnable(GL_DEPTH_TEST);

    m_flatShader.bind();
    m_flatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
    m_flatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
    m_flatShader.setUniformFloat("u_pointSize", Util::RenderStyle::VERTEX_POINT_SIZE);

    // --- Geometry edges (indexed GL_LINES) ---
    if(!lineRanges.empty() && geomBuffer.vertexCount() > 0) {
        geomBuffer.bindForDraw();

        f->glLineWidth(Util::RenderStyle::EDGE_LINE_WIDTH);
        std::vector<GLsizei> counts;
        std::vector<const void*> offsets;
        PassUtil::buildIndexedBatch(
            lineRanges, [](const DrawRangeEx&) { return true; }, counts, offsets);
        PassUtil::multiDrawElements(ctx, f, GL_LINES, counts, offsets);
        f->glLineWidth(1.0f);

        geomBuffer.unbind();
    }

    // --- Geometry points (GL_POINTS) ---
    if(!pointRanges.empty() && geomBuffer.vertexCount() > 0) {
        geomBuffer.bindForDraw();

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        m_flatShader.setUniformFloat("u_pointSize", Util::RenderStyle::VERTEX_POINT_SIZE);

        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            pointRanges, [](const DrawRangeEx&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx, f, GL_POINTS, firsts, counts);

        geomBuffer.unbind();
    }

    // --- Mesh wireframe edges (array GL_LINES) ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Wireframe) && meshWireframeCount > 0 &&
       meshBuffer.vertexCount() > 0) {
        meshBuffer.bindForDraw();

        f->glLineWidth(1.0f);
        f->glDrawArrays(GL_LINES, static_cast<GLint>(meshSurfaceCount),
                        static_cast<GLsizei>(meshWireframeCount));

        meshBuffer.unbind();
    }

    // --- Mesh node points (array GL_POINTS) ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Points) && meshNodeCount > 0 &&
       meshBuffer.vertexCount() > 0) {
        meshBuffer.bindForDraw();

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        m_flatShader.setUniformFloat("u_pointSize", 3.0f);
        f->glDrawArrays(GL_POINTS, static_cast<GLint>(meshSurfaceCount + meshWireframeCount),
                        static_cast<GLsizei>(meshNodeCount));

        meshBuffer.unbind();
    }

    m_flatShader.release();
}

} // namespace OpenGeoLab::Render
