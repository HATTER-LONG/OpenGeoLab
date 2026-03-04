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
uniform float u_viewOffset;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_PointSize = u_pointSize;
    vec4 viewPos = u_viewMatrix * vec4(a_position, 1.0);
    viewPos.z -= u_viewOffset;
    gl_Position = u_projMatrix * viewPos;
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

const float kMeshViewOffset = -1.0f;

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

void WireframePass::render(const RenderPassContext& ctx_data) {
    const auto& params = ctx_data.m_params;
    if(!isInitialized()) {
        return;
    }

    if(ctx_data.m_geometry.m_buffer == nullptr || ctx_data.m_mesh.m_buffer == nullptr ||
       ctx_data.m_geometry.m_lineRanges == nullptr ||
       ctx_data.m_geometry.m_pointRanges == nullptr || ctx_data.m_mesh.m_lineRanges == nullptr ||
       ctx_data.m_mesh.m_pointRanges == nullptr) {
        return;
    }

    auto& geomBuffer = *ctx_data.m_geometry.m_buffer;
    auto& meshBuffer = *ctx_data.m_mesh.m_buffer;
    const auto& lineRanges = *ctx_data.m_geometry.m_lineRanges;
    const auto& pointRanges = *ctx_data.m_geometry.m_pointRanges;
    const auto& meshLineRanges = *ctx_data.m_mesh.m_lineRanges;
    const auto& meshPointRanges = *ctx_data.m_mesh.m_pointRanges;
    const auto meshDisplayMode = ctx_data.m_mesh.m_displayMode;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    f->glEnable(GL_DEPTH_TEST);

    m_flatShader.bind();
    m_flatShader.setUniformMatrix4("u_viewMatrix", params.viewMatrix);
    m_flatShader.setUniformMatrix4("u_projMatrix", params.projMatrix);
    m_flatShader.setUniformFloat("u_pointSize", Util::RenderStyle::VERTEX_POINT_SIZE);
    m_flatShader.setUniformFloat("u_viewOffset", 0.0f);

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
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Wireframe) && !meshLineRanges.empty() &&
       meshBuffer.vertexCount() > 0) {
        m_flatShader.setUniformFloat("u_viewOffset", kMeshViewOffset);
        meshBuffer.bindForDraw();

        f->glLineWidth(1.0f);
        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            meshLineRanges, [](const DrawRangeEx&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx, f, GL_LINES, firsts, counts);

        meshBuffer.unbind();
        m_flatShader.setUniformFloat("u_viewOffset", 0.0f);
    }

    // --- Mesh node points (array GL_POINTS) ---
    if(hasMode(meshDisplayMode, RenderDisplayModeMask::Points) && !meshPointRanges.empty() &&
       meshBuffer.vertexCount() > 0) {
        m_flatShader.setUniformFloat("u_viewOffset", kMeshViewOffset);
        meshBuffer.bindForDraw();

        f->glEnable(GL_PROGRAM_POINT_SIZE);
        m_flatShader.setUniformFloat("u_pointSize", 3.0f);
        std::vector<GLint> firsts;
        std::vector<GLsizei> counts;
        PassUtil::buildArrayBatch(
            meshPointRanges, [](const DrawRangeEx&) { return true; }, firsts, counts);
        PassUtil::multiDrawArrays(ctx, f, GL_POINTS, firsts, counts);

        meshBuffer.unbind();
        m_flatShader.setUniformFloat("u_viewOffset", 0.0f);
    }

    m_flatShader.release();
}

} // namespace OpenGeoLab::Render
