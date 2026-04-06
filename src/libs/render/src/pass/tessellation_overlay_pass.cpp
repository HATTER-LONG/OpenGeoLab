#include "pass/tessellation_overlay_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/render/batch_utils.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view TESS_OVERLAY_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvp;
uniform float u_pointSize;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    gl_PointSize = u_pointSize;
}
)glsl";

constexpr std::string_view TESS_OVERLAY_FS = R"glsl(
#version 330 core
uniform vec4 u_color;
out vec4 fragColor;
void main() {
    fragColor = u_color;
}
)glsl";

/// Semi-transparent white for triangle edge lines.
constexpr glm::vec4 EDGE_COLOR{1.0F, 1.0F, 1.0F, 0.6F};

/// Opaque purple for tessellation vertex dots.
constexpr glm::vec4 VERTEX_COLOR{0.6F, 0.2F, 0.9F, 1.0F};

/// Base point size in logical pixels (scaled by devicePixelRatio).
constexpr float POINT_SIZE = 3.0F;

} // namespace

bool TessellationOverlayPass::onInitialize() {
    return m_shader.create(TESS_OVERLAY_VS, TESS_OVERLAY_FS);
}

void TessellationOverlayPass::onCleanup() { m_shader.destroy(); }

void TessellationOverlayPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if(!state.showTessellation || !buffers.hasData()) {
        return;
    }

    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const auto edge_batch =
        BatchUtils::buildIndexedBatch(buffers.triangleRanges(), [](const Scene::DrawRange&) {
            return true;
        });
    const auto point_batch =
        BatchUtils::buildIndexedBatch(buffers.triangleRanges(), [](const Scene::DrawRange&) {
            return true;
        });

    m_shader.use();
    m_shader.setMat4("u_mvp", mvp);

    buffers.bindMainVao();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0F, -1.0F);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0F);

    m_shader.setVec4("u_color", EDGE_COLOR);
    m_shader.setFloat("u_pointSize", 1.0F);
    BatchUtils::multiDrawElements(GL_TRIANGLES, edge_batch);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);

    glEnable(GL_PROGRAM_POINT_SIZE);
    m_shader.setVec4("u_color", VERTEX_COLOR);
    m_shader.setFloat("u_pointSize", POINT_SIZE * state.devicePixelRatio);
    BatchUtils::multiDrawElements(GL_POINTS, point_batch);

    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);

    buffers.unbind();
}

} // namespace OpenGeoLab::Render
