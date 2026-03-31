#include "pass/wireframe_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/render/batch_utils.hpp>
#include <opengeolab/scene/display_mode.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view LINE_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
out vec4 v_color;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_color = a_color;
}
)glsl";

constexpr std::string_view POINT_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
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

constexpr float LINE_WIDTH = 1.5F;
constexpr float DEFAULT_LINE_WIDTH = 1.0F;
constexpr float POINT_SIZE = 6.0F;

} // namespace

bool WireframePass::onInitialize() {
    if(!m_lineShader.create(LINE_VS, WIREFRAME_FS)) {
        return false;
    }

    if(!m_pointShader.create(POINT_VS, WIREFRAME_FS)) {
        m_lineShader.destroy();
        return false;
    }

    return true;
}

void WireframePass::onCleanup() {
    m_pointShader.destroy();
    m_lineShader.destroy();
}

void WireframePass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if(!buffers.hasData()) {
        return;
    }

    using Scene::DisplayModeMask;
    if((state.displayMask & DisplayModeMask::Wireframe) == DisplayModeMask::None) {
        return;
    }

    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    constexpr float ALPHA = 1.0f;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(LINE_WIDTH);

    buffers.bindMainVao();

    m_lineShader.use();
    m_lineShader.setMat4("u_mvp", mvp);
    m_lineShader.setFloat("u_alpha", ALPHA);
    const auto line_batch = BatchUtils::buildIndexedBatch(
        buffers.lineRanges(), [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawElements(GL_LINES, line_batch);

    glEnable(GL_PROGRAM_POINT_SIZE);

    m_pointShader.use();
    m_pointShader.setMat4("u_mvp", mvp);
    m_pointShader.setFloat("u_alpha", ALPHA);
    m_pointShader.setFloat("u_pointSize", POINT_SIZE);
    const auto point_batch = BatchUtils::buildArrayBatch(
        buffers.pointRanges(), [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawArrays(GL_POINTS, point_batch);

    buffers.unbind();

    glDisable(GL_PROGRAM_POINT_SIZE);
    glLineWidth(DEFAULT_LINE_WIDTH);
    glDepthFunc(GL_LESS);
}

} // namespace OpenGeoLab::Render
