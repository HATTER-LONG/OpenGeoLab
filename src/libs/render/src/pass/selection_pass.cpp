#include "pass/selection_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/render/batch_utils.hpp>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view PICK_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in uvec2 a_pickId;
uniform mat4 u_mvp;
uniform float u_pointSize;
flat out uvec2 v_pickId;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    gl_PointSize = u_pointSize;
    v_pickId = a_pickId;
}
)glsl";

constexpr std::string_view PICK_FS = R"glsl(
#version 330 core
flat in uvec2 v_pickId;
layout(location = 0) out uvec2 fragPickId;
void main() {
    fragPickId = v_pickId;
}
)glsl";

constexpr float PICK_LINE_WIDTH = 4.0F;
constexpr float DEFAULT_LINE_WIDTH = 1.0F;
constexpr float PICK_POINT_SIZE = 12.0F;

} // namespace

bool SelectionPass::onInitialize() { return m_shader.create(PICK_VS, PICK_FS); }

void SelectionPass::onCleanup() {
    m_pickFbo.cleanup();
    m_pickFboInitialized = false;
    m_shader.destroy();
}

void SelectionPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if(!buffers.hasData()) {
        return;
    }

    if(state.viewportWidth <= 0 || state.viewportHeight <= 0) {
        return;
    }

    if(!m_pickFboInitialized) {
        m_pickFbo.cleanup();
        if(!m_pickFbo.initialize(state.viewportWidth, state.viewportHeight)) {
            m_pickFbo.cleanup();
            return;
        }
        m_pickFboInitialized = true;
    } else {
        m_pickFbo.resize(state.viewportWidth, state.viewportHeight);
    }

    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const GLuint clear_value[4] = {0U, 0U, 0U, 0U};

    m_pickFbo.bind();
    glClearBufferuiv(GL_COLOR, 0, clear_value);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    m_shader.use();
    m_shader.setMat4("u_mvp", mvp);

    buffers.bindPickVao();

    m_shader.setFloat("u_pointSize", PICK_POINT_SIZE);

    const auto triangle_batch = BatchUtils::buildIndexedBatch(
        buffers.triangleRanges(), [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawElements(GL_TRIANGLES, triangle_batch);

    // GL_LEQUAL so edges/vertices at equal depth can overwrite face pick IDs.
    glDepthFunc(GL_LEQUAL);

    glLineWidth(PICK_LINE_WIDTH);
    const auto line_batch = BatchUtils::buildIndexedBatch(
        buffers.lineRanges(), [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawElements(GL_LINES, line_batch);

    glEnable(GL_PROGRAM_POINT_SIZE);
    const auto point_batch = BatchUtils::buildArrayBatch(
        buffers.pointRanges(), [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawArrays(GL_POINTS, point_batch);

    buffers.unbind();
    m_pickFbo.unbind();

    glDisable(GL_PROGRAM_POINT_SIZE);
    glLineWidth(DEFAULT_LINE_WIDTH);
    glDepthFunc(GL_LESS);
    glViewport(0, 0, state.viewportWidth, state.viewportHeight);
}

} // namespace OpenGeoLab::Render
