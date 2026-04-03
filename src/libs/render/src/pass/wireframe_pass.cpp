#include "pass/wireframe_pass.hpp"

#include "core/gpu_buffer_manager.hpp"
#include "core/thick_line_renderer.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/render/batch_utils.hpp>
#include <opengeolab/scene/display_mode.hpp>

#include <glad/gl.h>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view POINT_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    vec4 pos = u_mvp * vec4(a_position, 1.0);
    pos.z -= 0.0006 * pos.w;
    gl_Position = pos;
    gl_PointSize = u_pointSize;
    v_color = a_color;
}
)glsl";

constexpr std::string_view WIREFRAME_FS = R"glsl(
#version 330 core
in vec4 v_color;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    float a = v_color.a * u_alpha;
    fragColor = vec4(v_color.rgb * a, a);
}
)glsl";

constexpr float POINT_SIZE = 6.0F;
constexpr float MESH_POINT_SIZE = 3.0F;
constexpr float MESH_EDGE_WIDTH_FACTOR = 0.6F;

} // namespace

bool WireframePass::onInitialize() { return m_pointShader.create(POINT_VS, WIREFRAME_FS); }

void WireframePass::onCleanup() { m_pointShader.destroy(); }

void WireframePass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if(!buffers.hasData()) {
        return;
    }

    using Scene::DisplayModeMask;
    if((state.displayMask & DisplayModeMask::Wireframe) == DisplayModeMask::None) {
        return;
    }

    const auto& cfg = Core::ColorMap::active();
    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    constexpr float alpha = 1.0F;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // --- Geometry edges via ThickLineRenderer ---
    if(m_thickLine != nullptr) {
        const glm::vec2 viewport{static_cast<float>(state.viewportWidth) * state.devicePixelRatio,
                                 static_cast<float>(state.viewportHeight) * state.devicePixelRatio};

        const auto geo_lines =
            BatchUtils::filterRanges(buffers.lineRanges(), [](const Scene::DrawRange& r) {
                return r.entityType != Core::EntityType::MeshEdge;
            });
        if(!geo_lines.empty()) {
            m_thickLine->drawLines({.positionVbo = buffers.mainVbo(),
                                    .indexBuffer = buffers.ibo(),
                                    .mvp = mvp,
                                    .viewport = viewport,
                                    .lineWidth = cfg.defaultEdgeWidth * state.devicePixelRatio,
                                    .color = {},
                                    .useVertexColor = true,
                                    .colorMix = 0.0F,
                                    .depthBias = 0.0003F},
                                   geo_lines);
        }

        const auto mesh_lines =
            BatchUtils::filterRanges(buffers.lineRanges(), [](const Scene::DrawRange& r) {
                return r.entityType == Core::EntityType::MeshEdge;
            });
        if(!mesh_lines.empty()) {
            m_thickLine->drawLines({.positionVbo = buffers.mainVbo(),
                                    .indexBuffer = buffers.ibo(),
                                    .mvp = mvp,
                                    .viewport = viewport,
                                    .lineWidth = cfg.defaultEdgeWidth * MESH_EDGE_WIDTH_FACTOR *
                                                 state.devicePixelRatio,
                                    .color = {},
                                    .useVertexColor = true,
                                    .colorMix = 0.0F,
                                    .depthBias = 0.0002F},
                                   mesh_lines);
        }
    }

    // --- Geometry points ---
    buffers.bindMainVao();
    glEnable(GL_PROGRAM_POINT_SIZE);

    m_pointShader.use();
    m_pointShader.setMat4("u_mvp", mvp);
    m_pointShader.setFloat("u_alpha", alpha);

    m_pointShader.setFloat("u_pointSize", POINT_SIZE);
    const auto geo_points =
        BatchUtils::buildArrayBatch(buffers.pointRanges(), [](const Scene::DrawRange& r) {
            return r.entityType != Core::EntityType::MeshNode;
        });
    BatchUtils::multiDrawArrays(GL_POINTS, geo_points);

    // --- Mesh nodes (smaller green points) ---
    m_pointShader.setFloat("u_pointSize", MESH_POINT_SIZE);
    const auto mesh_points =
        BatchUtils::buildArrayBatch(buffers.pointRanges(), [](const Scene::DrawRange& r) {
            return r.entityType == Core::EntityType::MeshNode;
        });
    BatchUtils::multiDrawArrays(GL_POINTS, mesh_points);

    buffers.unbind();
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDepthFunc(GL_LESS);
}

} // namespace OpenGeoLab::Render
