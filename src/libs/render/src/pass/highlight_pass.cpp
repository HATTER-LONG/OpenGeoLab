#include "pass/highlight_pass.hpp"

#include "core/gpu_buffer_manager.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/render/batch_utils.hpp>

#include <glad/gl.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <string_view>

namespace OpenGeoLab::Render {

namespace {

// --- Shaders ---

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
    vec4 pos = u_mvp * vec4(a_position, 1.0);
    // Depth bias: push highlighted edges slightly towards camera so that
    // wide-line extra-width fragments pass GL_LEQUAL against face depth.
    pos.z -= 0.0005 * pos.w;
    gl_Position = pos;
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

constexpr std::string_view HIGHLIGHT_POINT_VS = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 2) in vec4 a_color;
uniform mat4 u_mvp;
uniform float u_pointSize;
out vec4 v_color;
void main() {
    vec4 pos = u_mvp * vec4(a_position, 1.0);
    pos.z -= 0.0005 * pos.w;
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

constexpr float DEFAULT_LINE_WIDTH = 1.0F;

/// Bundled transform matrices for face highlight rendering.
struct FaceTransforms {
    glm::mat4 mvp;
    glm::mat4 modelView;
    glm::mat3 normalMatrix;
};

/// Convert Core::RenderColor to glm::vec4.
glm::vec4 toVec4(const Core::RenderColor& c) { return {c.r, c.g, c.b, c.a}; }

/// Check whether an entity type represents face/solid geometry.
bool isFaceType(Core::EntityType t) {
    return t == Core::EntityType::GeoFace || t == Core::EntityType::GeoSolid;
}

/// Check whether an entity type represents edge/wire geometry.
bool isEdgeType(Core::EntityType t) {
    return t == Core::EntityType::GeoEdge || t == Core::EntityType::GeoWire;
}

/// Check whether an entity type represents vertex geometry.
bool isVertexType(Core::EntityType t) { return t == Core::EntityType::GeoVertex; }

/// Partition highlight entries into face, edge, and vertex range lists.
struct PartitionedRanges {
    std::vector<Scene::DrawRange> faces;
    std::vector<Scene::DrawRange> edges;
    std::vector<Scene::DrawRange> vertices;
};

PartitionedRanges partition(const std::vector<HighlightEntry>& entries) {
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

} // namespace

bool HighlightPass::onInitialize() {
    if(!m_faceShader.create(HIGHLIGHT_FACE_VS, HIGHLIGHT_FACE_FS)) {
        return false;
    }
    if(!m_edgeShader.create(HIGHLIGHT_EDGE_VS, HIGHLIGHT_EDGE_FS)) {
        m_faceShader.destroy();
        return false;
    }
    if(!m_pointShader.create(HIGHLIGHT_POINT_VS, HIGHLIGHT_POINT_FS)) {
        m_edgeShader.destroy();
        m_faceShader.destroy();
        return false;
    }
    return true;
}

void HighlightPass::onCleanup() {
    m_pointShader.destroy();
    m_edgeShader.destroy();
    m_faceShader.destroy();
}

void HighlightPass::render(const FrameState& state, const GpuBufferManager& buffers) {
    if((state.selectedEntries.empty() && state.hoveredEntries.empty()) || !buffers.hasData()) {
        return;
    }

    const auto& cfg = Core::ColorMap::active();
    const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
    const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(state.viewMatrix));
    const FaceTransforms transforms{mvp, state.viewMatrix, normal_matrix};
    constexpr float alpha = 1.0F;

    buffers.bindMainVao();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // --- Selection highlight ---
    {
        auto [faces, edges, vertices] = partition(state.selectedEntries);
        std::fprintf(stderr, "[Highlight] sel: faces=%zu edges=%zu verts=%zu\n",
                     faces.size(), edges.size(), vertices.size());

        if(!faces.empty()) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0F, 5.0F);
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            m_faceShader.setMat4("u_modelView", transforms.modelView);
            const GLint loc = glGetUniformLocation(m_faceShader.id(), "u_normalMatrix");
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.selectionFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        if(!edges.empty()) {
            glLineWidth(cfg.selectionEdgeVertex.lineWidth);
            m_edgeShader.use();
            m_edgeShader.setMat4("u_mvp", mvp);
            m_edgeShader.setVec4("u_highlightColor", toVec4(cfg.selectionEdgeVertex.color));
            m_edgeShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                edges, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_LINES, batch);
        }

        if(!vertices.empty()) {
            const float pt_size = cfg.defaultPointSize * cfg.selectionEdgeVertex.pointScale;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.selectionEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildArrayBatch(
                vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    // --- Hover highlight ---
    {
        auto [faces, edges, vertices] = partition(state.hoveredEntries);
        if(!faces.empty() || !edges.empty() || !vertices.empty()) {
            std::fprintf(stderr, "[Highlight] hov: faces=%zu edges=%zu verts=%zu\n",
                         faces.size(), edges.size(), vertices.size());
        }

        if(!faces.empty()) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0F, 5.0F);
            m_faceShader.use();
            m_faceShader.setMat4("u_mvp", transforms.mvp);
            m_faceShader.setMat4("u_modelView", transforms.modelView);
            const GLint loc = glGetUniformLocation(m_faceShader.id(), "u_normalMatrix");
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(transforms.normalMatrix));
            m_faceShader.setVec4("u_highlightColor", toVec4(cfg.hoverFace.color));
            m_faceShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                faces, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_TRIANGLES, batch);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        if(!edges.empty()) {
            glLineWidth(cfg.hoverEdgeVertex.lineWidth);
            m_edgeShader.use();
            m_edgeShader.setMat4("u_mvp", mvp);
            m_edgeShader.setVec4("u_highlightColor", toVec4(cfg.hoverEdgeVertex.color));
            m_edgeShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildIndexedBatch(
                edges, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawElements(GL_LINES, batch);
        }

        if(!vertices.empty()) {
            const float pt_size = cfg.defaultPointSize * cfg.hoverEdgeVertex.pointScale;
            glEnable(GL_PROGRAM_POINT_SIZE);
            m_pointShader.use();
            m_pointShader.setMat4("u_mvp", mvp);
            m_pointShader.setFloat("u_pointSize", pt_size);
            m_pointShader.setVec4("u_highlightColor", toVec4(cfg.hoverEdgeVertex.color));
            m_pointShader.setFloat("u_alpha", alpha);
            const auto batch = BatchUtils::buildArrayBatch(
                vertices, [](const Scene::DrawRange&) { return true; });
            BatchUtils::multiDrawArrays(GL_POINTS, batch);
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }

    glLineWidth(DEFAULT_LINE_WIDTH);
    glDepthFunc(GL_LESS);
    buffers.unbind();
}

} // namespace OpenGeoLab::Render
