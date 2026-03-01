/**
 * @file selection_pass.cpp
 * @brief SelectionPass implementation — offscreen FBO picking with integer IDs.
 *
 * Renamed from PickPass. Renders entity IDs to an offscreen framebuffer
 * for GPU-based selection and hover detection.
 */

#include "selection_pass.hpp"
#include "draw_batch_utils.hpp"
#include "render/core/gpu_buffer.hpp"

#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>

namespace OpenGeoLab::Render {

namespace {

const char* PICK_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 3) in uvec2 a_pickId;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;
uniform float u_pointSize;
flat out uvec2 v_pickId;
void main() {
    v_pickId = a_pickId;
    gl_PointSize = u_pointSize;
    gl_Position = u_projMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
)";

const char* PICK_FRAGMENT_SHADER = R"(
#version 330 core
flat in uvec2 v_pickId;
layout(location = 0) out uvec2 fragPickId;
void main() {
    fragPickId = v_pickId;
}
)";

[[nodiscard]] constexpr bool hasAny(RenderEntityTypeMask value, RenderEntityTypeMask mask) {
    return static_cast<uint32_t>(value & mask) != 0u;
}

/// Triangle-based geometry types
constexpr auto TRIANGLE_PICK_TYPES = RenderEntityTypeMask::Face | RenderEntityTypeMask::Shell |
                                     RenderEntityTypeMask::Solid | RenderEntityTypeMask::Part |
                                     RenderEntityTypeMask::Wire;

/// Mesh types that use the mesh buffer
constexpr auto MESH_PICK_TYPES =
    RenderEntityTypeMask::MeshNode | RenderEntityTypeMask::MeshLine | RENDER_MESH_ELEMENTS;

} // anonymous namespace

void SelectionPass::initialize(int width, int height) {
    if(m_initialized) {
        return;
    }

    if(!m_pickShader.compile(PICK_VERTEX_SHADER, PICK_FRAGMENT_SHADER)) {
        LOG_ERROR("SelectionPass: Failed to compile pick shader");
        return;
    }

    m_fbo.initialize(width, height);
    m_initialized = true;
    LOG_DEBUG("SelectionPass: Initialized ({}x{})", width, height);
}

void SelectionPass::resize(int width, int height) {
    if(!m_initialized) {
        return;
    }
    m_fbo.resize(width, height);
    LOG_DEBUG("SelectionPass: Resized to {}x{}", width, height);
}

void SelectionPass::cleanup() {
    m_fbo.cleanup();
    m_initialized = false;
    LOG_DEBUG("SelectionPass: Cleaned up");
}

void SelectionPass::renderToFbo(const QMatrix4x4& view,
                                const QMatrix4x4& projection,
                                GpuBuffer& geomBuffer,
                                const std::vector<DrawRangeEx>& triRanges,
                                const std::vector<DrawRangeEx>& lineRanges,
                                const std::vector<DrawRangeEx>& pointRanges,
                                GpuBuffer& meshBuffer,
                                uint32_t meshSurfaceCount,
                                uint32_t meshWireframeCount,
                                uint32_t meshNodeCount,
                                RenderEntityTypeMask pickMask) {
    if(!m_initialized) {
        LOG_ERROR("SelectionPass: Not initialized");
        return;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    m_fbo.bind();

    const GLuint clearColor[4] = {0, 0, 0, 0};
    ef->glClearBufferuiv(GL_COLOR, 0, clearColor);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);

    m_pickShader.bind();
    m_pickShader.setUniformMatrix4("u_viewMatrix", view);
    m_pickShader.setUniformMatrix4("u_projMatrix", projection);
    m_pickShader.setUniformFloat("u_pointSize", 1.0f);

    // --- Geometry buffer (per-entity selective rendering) ---
    if(geomBuffer.vertexCount() > 0) {
        geomBuffer.bindForDraw();

        // Triangles
        if(hasAny(pickMask, TRIANGLE_PICK_TYPES) && !triRanges.empty()) {
            std::vector<GLsizei> counts;
            std::vector<const void*> offsets;
            PassUtil::buildIndexedBatch(
                triRanges,
                [&pickMask](const DrawRangeEx& rangeEx) {
                    return hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type));
                },
                counts, offsets);
            PassUtil::multiDrawElements(ctx, f, GL_TRIANGLES, counts, offsets);
        }

        // Lines
        if(hasAny(pickMask, RenderEntityTypeMask::Edge) && !lineRanges.empty()) {
            f->glLineWidth(3.0f);
            std::vector<GLsizei> counts;
            std::vector<const void*> offsets;
            PassUtil::buildIndexedBatch(
                lineRanges,
                [&pickMask](const DrawRangeEx& rangeEx) {
                    return hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type));
                },
                counts, offsets);
            PassUtil::multiDrawElements(ctx, f, GL_LINES, counts, offsets);
            f->glLineWidth(1.0f);
        }

        // Points
        if(hasAny(pickMask, RenderEntityTypeMask::Vertex) && !pointRanges.empty()) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f);

            std::vector<GLint> firsts;
            std::vector<GLsizei> counts;
            PassUtil::buildArrayBatch(
                pointRanges,
                [&pickMask](const DrawRangeEx& rangeEx) {
                    return hasAny(pickMask, toMask(rangeEx.m_entityKey.m_type));
                },
                firsts, counts);
            PassUtil::multiDrawArrays(ctx, f, GL_POINTS, firsts, counts);

            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }

        geomBuffer.unbind();
    }

    // --- Mesh buffer (selective rendering per mesh topology) ---
    if(hasAny(pickMask, MESH_PICK_TYPES) && meshBuffer.vertexCount() > 0) {
        meshBuffer.bindForDraw();

        if(hasAny(pickMask, RENDER_MESH_ELEMENTS) && meshSurfaceCount > 0) {
            f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(meshSurfaceCount));
        }

        if(hasAny(pickMask, RenderEntityTypeMask::MeshLine) && meshWireframeCount > 0) {
            f->glLineWidth(3.0f);
            f->glDrawArrays(GL_LINES, static_cast<GLint>(meshSurfaceCount),
                            static_cast<GLsizei>(meshWireframeCount));
            f->glLineWidth(1.0f);
        }

        if(hasAny(pickMask, RenderEntityTypeMask::MeshNode) && meshNodeCount > 0) {
            f->glEnable(GL_PROGRAM_POINT_SIZE);
            m_pickShader.setUniformFloat("u_pointSize", 12.0f);
            f->glDrawArrays(GL_POINTS, static_cast<GLint>(meshSurfaceCount + meshWireframeCount),
                            static_cast<GLsizei>(meshNodeCount));
            m_pickShader.setUniformFloat("u_pointSize", 1.0f);
        }

        meshBuffer.unbind();
    }

    m_pickShader.release();
    m_fbo.unbind();
}

uint64_t SelectionPass::readPickId(int pixelX, int pixelY) const {
    return m_fbo.readPickId(pixelX, pixelY);
}

std::vector<uint64_t> SelectionPass::readPickRegion(int cx, int cy, int radius) const {
    return m_fbo.readPickRegion(cx, cy, radius);
}

} // namespace OpenGeoLab::Render
