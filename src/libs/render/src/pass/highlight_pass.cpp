#include "pass/highlight_pass.hpp"

#include "core/gpu_buffer_manager.hpp"
#include "core/thick_line_renderer.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/render/batch_utils.hpp>

#include <glad/gl.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

namespace {

// --- Shaders (face + point only; edges use ThickLineRenderer) ---

constexpr std::string_view HIGHLIGHT_FACE_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform mat3 u_normalMatrix;
out vec3 v_normal;
out vec4 v_color;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = normalize(u_normalMatrix * a_normal);
    v_color = a_color;
}
)glsl";

constexpr std::string_view HIGHLIGHT_FACE_FS = R"glsl(
#version 330 core
in vec3 v_normal;
in vec4 v_color;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 N = normalize(v_normal);
    // Orthographic headlamp: constant view direction along +Z in view space.
    vec3 V = vec3(0.0, 0.0, 1.0);
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

constexpr std::string_view HIGHLIGHT_POINT_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    vec4 pos = u_mvp * vec4(a_position, 1.0);
    pos.z -= 0.003 * pos.w;
    gl_Position = pos;
    gl_PointSize = u_pointSize;
    v_color = a_color;
}
)glsl";

constexpr std::string_view HIGHLIGHT_POINT_FS = R"glsl(
#version 330 core
in vec4 v_color;
uniform vec4 u_highlightColor;
uniform float u_alpha;
out vec4 fragColor;
void main() {
    vec3 finalColor = mix(v_color.rgb, u_highlightColor.rgb, u_highlightColor.a);
    fragColor = vec4(finalColor, u_alpha);
}
)glsl";

/// Bundled transform matrices for face highlight rendering.
struct FaceTransforms {
    glm::mat4 mvp;
    glm::mat3 normalMatrix;
};

/// Partition highlight entries into face, edge, and vertex range lists.
struct PartitionedRanges {
    std::vector<Scene::DrawRange> faces;
    std::vector<Scene::DrawRange> edges;
    std::vector<Scene::DrawRange> vertices;
};

} // namespace

/// Convert Core::RenderColor to glm::vec4.
static glm::vec4 toVec4(const Core::RenderColor& c) { return {c.r, c.g, c.b, c.a}; }

/// Check whether an entity type represents face/solid geometry.
static bool isFaceType(Core::EntityType t) {
    return t == Core::EntityType::GeoFace || t == Core::EntityType::GeoSolid ||
           t == Core::EntityType::MeshElement;
}

/// Check whether an entity type represents edge/wire geometry.
static bool isEdgeType(Core::EntityType t) {
    return t == Core::EntityType::GeoEdge || t == Core::EntityType::GeoWire ||
           t == Core::EntityType::MeshEdge;
}

/// Check whether an entity type represents vertex geometry.
static bool isVertexType(Core::EntityType t) {
    return t == Core::EntityType::GeoVertex || t == Core::EntityType::MeshNode;
}

static PartitionedRanges partition(const std::vector<HighlightEntry>& entries) {
    PartitionedRanges out;
    for(const auto& e : entries) {
        if(isFaceType(e.entityType)) {
            out.faces.push_back(e.range);
        } else if(isEdgeType(e.entityType)) {
            out.edges.push_back(e.range);
        } else if(isVertexType(e.entityType)) {
            out.vertices.push_back(e.range);
        }
    }
    return out;
}

bool HighlightPass::onInitialize() {
    if(!m_faceShader.create(HIGHLIGHT_FACE_VS, HIGHLIGHT_FACE_FS)) {
        return false;
    }
    if(!m_pointShader.create(HIGHLIGHT_POINT_VS, HIGHLIGHT_POINT_FS)) {
        m_faceShader.destroy();
        return false;
    }
    m_normalMatrixLoc = glGetUniformLocation(m_faceShader.id(), "u_normalMatrix");
    return true;
}

void HighlightPass::onCleanup() {
    m_pointShader.destroy();
    m_faceShader.destroy();
}

void HighlightPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if((state.selectedEntries.empty() && state.hoveredEntries.empty()) || !buffers.hasData()) {
        return;
    }

    const auto& cfg = Core::ColorMap::active();
    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(state.viewMatrix));
    const FaceTransforms transforms{mvp, normal_matrix};
    constexpr float alpha = 1.0F;

    const glm::vec2 viewport{static_cast<float>(state.viewportWidth),
                             static_cast<float>(state.viewportHeight)};

    buffers.bindMainVao();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // --- Selection highlight ---
    {
        auto [faces, edges, vertices] = partition(state.selectedEntries);

        if(!faces.empty()) {
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            glUniformMatrix3fv(m_normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.selectionFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch =
                BatchUtils::buildIndexedBatch(faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
        }

        if(!edges.empty() && m_thickLine != nullptr) {
            buffers.unbind();
            m_thickLine->drawLines(
                {.positionVbo = buffers.mainVbo(),
                 .indexBuffer = buffers.ibo(),
                 .mvp = mvp,
                 .viewport = viewport,
                 .lineWidth = cfg.selectionEdgeVertex.lineWidth * state.devicePixelRatio,
                 .color = toVec4(cfg.selectionEdgeVertex.color),
                 .useVertexColor = false,
                 .colorMix = 0.0F,
                 .depthBias = 0.001F},
                edges);
            buffers.bindMainVao();
        }

        if(!vertices.empty()) {
            const float pt_size =
                cfg.defaultPointSize * cfg.selectionEdgeVertex.pointScale * state.devicePixelRatio;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.selectionEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch =
                BatchUtils::buildArrayBatch(vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    // --- Hover highlight ---
    {
        auto [faces, edges, vertices] = partition(state.hoveredEntries);

        if(!faces.empty()) {
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            glUniformMatrix3fv(m_normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.hoverFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch =
                BatchUtils::buildIndexedBatch(faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
        }

        if(!edges.empty() && m_thickLine != nullptr) {
            buffers.unbind();
            m_thickLine->drawLines(
                {.positionVbo = buffers.mainVbo(),
                 .indexBuffer = buffers.ibo(),
                 .mvp = mvp,
                 .viewport = viewport,
                 .lineWidth = cfg.hoverEdgeVertex.lineWidth * state.devicePixelRatio,
                 .color = toVec4(cfg.hoverEdgeVertex.color),
                 .useVertexColor = false,
                 .colorMix = 0.0F,
                 .depthBias = 0.002F},
                edges);
            buffers.bindMainVao();
        }

        if(!vertices.empty()) {
            const float pt_size =
                cfg.defaultPointSize * cfg.hoverEdgeVertex.pointScale * state.devicePixelRatio;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.hoverEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch =
                BatchUtils::buildArrayBatch(vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    glDepthFunc(GL_LESS);
    buffers.unbind();
}

} // namespace OpenGeoLab::Render
