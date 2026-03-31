#include "pass/highlight_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/render/batch_utils.hpp>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view HIGHLIGHT_FACE_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform mat4 u_modelView;
uniform mat3 u_normalMatrix;
out vec3 v_normal;
out vec4 v_color;
out vec3 v_viewPos;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = normalize(u_normalMatrix * a_normal);
    v_color = a_color;
    v_viewPos = (u_modelView * vec4(a_position, 1.0)).xyz;
}
)glsl";

constexpr std::string_view HIGHLIGHT_FACE_FS = R"glsl(
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in vec3 v_viewPos;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(-v_viewPos);
    float ambient      = 0.35;
    float headlamp     = abs(dot(N, V));
    float skyLight     = max(dot(N, vec3(0.0, 1.0, 0.0)), 0.0) * 0.15;
    float groundBounce = max(dot(N, vec3(0.0, -1.0, 0.0)), 0.0) * 0.05;
    float lighting     = ambient + headlamp * 0.55 + skyLight + groundBounce;
    vec3 litColor = v_color.rgb * min(lighting, 1.0);
    vec3 finalColor = mix(litColor, u_highlightColor.rgb, u_highlightColor.a);
    fragColor = vec4(finalColor * u_alpha, u_alpha);
}
)glsl";

constexpr std::string_view HIGHLIGHT_EDGE_VS = R"glsl(
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

constexpr std::string_view HIGHLIGHT_EDGE_FS = R"glsl(
#version 330 core
in vec4 v_color;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 finalColor = mix(v_color.rgb, u_highlightColor.rgb, u_highlightColor.a);
    float a = v_color.a * u_alpha;
    fragColor = vec4(finalColor * a, a);
}
)glsl";

constexpr glm::vec4 SELECTED_COLOR{0.2f, 0.4f, 0.9f, 0.6f};
constexpr glm::vec4 HOVERED_COLOR{0.4f, 0.7f, 1.0f, 0.4f};
constexpr float HIGHLIGHT_LINE_WIDTH = 1.5f;
constexpr float DEFAULT_LINE_WIDTH = 1.0f;

void drawHighlightedFaces(ShaderProgram& shader,
                          const std::vector<Scene::DrawRange>& ranges,
                          const glm::mat4& mvp,
                          const glm::mat4& model_view,
                          const glm::mat3& normal_matrix,
                          const glm::vec4& highlight_color,
                          const float alpha) {
    if(ranges.empty()) {
        return;
    }

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 5.0f);

    shader.use();
    shader.setMat4("u_mvp", mvp);
    shader.setMat4("u_modelView", model_view);

    const GLint normal_matrix_location = glGetUniformLocation(shader.id(), "u_normalMatrix");
    glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

    shader.setVec4("u_highlightColor", highlight_color);
    shader.setFloat("u_alpha", alpha);

    const auto batch =
        BatchUtils::buildIndexedBatch(ranges, [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawElements(GL_TRIANGLES, batch);

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void drawHighlightedEdges(ShaderProgram& shader,
                          const std::vector<Scene::DrawRange>& ranges,
                          const glm::mat4& mvp,
                          const glm::vec4& highlight_color,
                          const float alpha) {
    if(ranges.empty()) {
        return;
    }

    shader.use();
    shader.setMat4("u_mvp", mvp);
    shader.setVec4("u_highlightColor", highlight_color);
    shader.setFloat("u_alpha", alpha);

    const auto batch =
        BatchUtils::buildIndexedBatch(ranges, [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawElements(GL_LINES, batch);
}

} // namespace

bool HighlightPass::onInitialize() {
    if(!m_faceShader.create(HIGHLIGHT_FACE_VS, HIGHLIGHT_FACE_FS)) {
        return false;
    }

    if(!m_edgeShader.create(HIGHLIGHT_EDGE_VS, HIGHLIGHT_EDGE_FS)) {
        m_faceShader.destroy();
        return false;
    }

    return true;
}

void HighlightPass::onCleanup() {
    m_edgeShader.destroy();
    m_faceShader.destroy();
}

void HighlightPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if((state.selectedDrawRanges.empty() && state.hoveredDrawRanges.empty()) ||
       !buffers.hasData()) {
        return;
    }

    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(state.viewMatrix));
    const float alpha = 1.0f;

    buffers.bindMainVao();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glLineWidth(HIGHLIGHT_LINE_WIDTH);

    drawHighlightedFaces(m_faceShader, state.selectedDrawRanges, mvp, state.viewMatrix,
                         normal_matrix, SELECTED_COLOR, alpha);
    drawHighlightedEdges(m_edgeShader, state.selectedDrawRanges, mvp, SELECTED_COLOR, alpha);

    drawHighlightedFaces(m_faceShader, state.hoveredDrawRanges, mvp, state.viewMatrix,
                         normal_matrix, HOVERED_COLOR, alpha);
    drawHighlightedEdges(m_edgeShader, state.hoveredDrawRanges, mvp, HOVERED_COLOR, alpha);

    glLineWidth(DEFAULT_LINE_WIDTH);
    glDepthFunc(GL_LESS);
    buffers.unbind();
}

} // namespace OpenGeoLab::Render
