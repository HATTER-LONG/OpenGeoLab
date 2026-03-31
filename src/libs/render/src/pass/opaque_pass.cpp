#include "pass/opaque_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/render/batch_utils.hpp>
#include <opengeolab/scene/display_mode.hpp>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

constexpr std::string_view OPAQUE_VS = R"glsl(
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

constexpr std::string_view OPAQUE_FS = R"glsl(
#version 330 core
in vec3 v_normal;
in vec4 v_color;
in vec3 v_viewPos;
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
    vec3 lit = v_color.rgb * min(lighting, 1.0);
    float a = v_color.a * u_alpha;
    fragColor = vec4(lit * a, a);
}
)glsl";

} // namespace

bool OpaquePass::onInitialize() { return m_shader.create(OPAQUE_VS, OPAQUE_FS); }

void OpaquePass::onCleanup() { m_shader.destroy(); }

void OpaquePass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if(!buffers.hasData()) {
        return;
    }

    using Scene::DisplayModeMask;
    if((state.displayMask & DisplayModeMask::Surface) == DisplayModeMask::None) {
        return;
    }

    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(state.viewMatrix));

    m_shader.use();
    m_shader.setMat4("u_mvp", mvp);
    m_shader.setMat4("u_modelView", state.viewMatrix);

    const GLint normal_matrix_location = glGetUniformLocation(m_shader.id(), "u_normalMatrix");
    glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, glm::value_ptr(normal_mat));

    m_shader.setFloat("u_alpha", state.xRayMode ? 0.25f : 1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 10.0f);

    if(state.xRayMode) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    }

    buffers.bindMainVao();

    const auto batch = BatchUtils::buildIndexedBatch(buffers.triangleRanges(),
                                                     [](const Scene::DrawRange&) { return true; });
    BatchUtils::multiDrawElements(GL_TRIANGLES, batch);

    buffers.unbind();

    glDisable(GL_POLYGON_OFFSET_FILL);
    if(state.xRayMode) {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

} // namespace OpenGeoLab::Render
