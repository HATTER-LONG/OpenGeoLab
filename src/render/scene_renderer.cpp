/**
 * @file scene_renderer.cpp
 * @brief Implementation of SceneRenderer for OpenGL geometry visualization
 */

#include "render/scene_renderer.hpp"
#include "util/logger.hpp"

#include <QOpenGLFramebufferObject>
#include <QVector4D>

namespace OpenGeoLab::Render {

// =============================================================================
// Shader Sources
// =============================================================================
namespace {

const char* const MESH_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

uniform mat4 uMVPMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;
uniform float uPointSize;

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vColor;

void main() {
    vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    // Keep as-is; normalize safely in fragment shader (some meshes may have zero normals).
    vNormal = uNormalMatrix * aNormal;
    vColor = aColor;
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
}
)";

const char* const MESH_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vColor;

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform int uUseLighting;
uniform int uUseOverrideColor;
uniform vec4 uOverrideColor;

out vec4 fragColor;

void main() {
    vec4 baseColor = (uUseOverrideColor != 0) ? uOverrideColor : vColor;

    // For edges/vertices we want stable “CAD-like” colors.
    // Also avoids undefined behavior when normals are zero.
    if(uUseLighting == 0) {
        fragColor = baseColor;
        return;
    }

    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);

    // Diffuse
    vec3 norm = vNormal;
    float nlen = length(norm);
    if(nlen < 1e-6) {
        norm = vec3(0.0, 0.0, 1.0);
    } else {
        norm = norm / nlen;
    }
    vec3 lightDir = normalize(uLightPos - vWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    // Specular
    float specularStrength = 0.3;
    vec3 viewDir = normalize(uViewPos - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = (ambient + diffuse + specular) * baseColor.rgb;
    fragColor = vec4(result, baseColor.a);
}
)";

const char* const PICK_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uMVPMatrix;
uniform float uPointSize;

void main() {
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
}
)";

const char* const PICK_EDGE_GEOMETRY_SHADER = R"(
#version 330 core

layout(line_strip) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 uViewport;   // framebuffer size in pixels
uniform float uThickness; // line thickness in pixels

void main() {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;

    // Avoid division by zero / degenerate segments.
    if(p0.w == 0.0 || p1.w == 0.0) {
        return;
    }

    vec2 n0 = p0.xy / p0.w;
    vec2 n1 = p1.xy / p1.w;
    vec2 d = (n1 - n0) * uViewport;
    float len = length(d);
    if(len < 1e-6) {
        return;
    }

    vec2 dir = d / len;
    vec2 n = vec2(-dir.y, dir.x);
    vec2 offset_ndc = (n * (uThickness * 0.5)) / uViewport;

    vec2 off0 = offset_ndc * p0.w;
    vec2 off1 = offset_ndc * p1.w;

    gl_Position = p0 + vec4(off0, 0.0, 0.0);
    EmitVertex();
    gl_Position = p0 - vec4(off0, 0.0, 0.0);
    EmitVertex();
    gl_Position = p1 + vec4(off1, 0.0, 0.0);
    EmitVertex();
    gl_Position = p1 - vec4(off1, 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
}
)";

const char* const PICK_FRAGMENT_SHADER = R"(
#version 330 core
uniform vec4 uPickColor;
out vec4 fragColor;
void main() {
    fragColor = uPickColor;
}
)";

} // namespace

// =============================================================================
// MeshBuffers Implementation
// =============================================================================

SceneRenderer::MeshBuffers::MeshBuffers()
    : m_vao(std::make_unique<QOpenGLVertexArrayObject>()),
      m_vbo(std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer)),
      m_ebo(std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer)) {}

SceneRenderer::MeshBuffers::~MeshBuffers() { destroy(); }

void SceneRenderer::MeshBuffers::destroy() {
    if(m_vao && m_vao->isCreated()) {
        m_vao->destroy();
    }
    if(m_vbo && m_vbo->isCreated()) {
        m_vbo->destroy();
    }
    if(m_ebo && m_ebo->isCreated()) {
        m_ebo->destroy();
    }
}

// =============================================================================
// SceneRenderer Implementation
// =============================================================================

SceneRenderer::SceneRenderer() { LOG_TRACE("SceneRenderer created"); }

SceneRenderer::~SceneRenderer() {
    cleanup();
    LOG_TRACE("SceneRenderer destroyed");
}

void SceneRenderer::initialize() {
    if(m_initialized) {
        return;
    }

    initializeOpenGLFunctions();
    setupShaders();

    m_initialized = true;
    LOG_DEBUG("SceneRenderer: OpenGL initialized");
}

void SceneRenderer::setViewportSize(const QSize& size) { m_viewportSize = size; }

void SceneRenderer::setupShaders() {
    // Mesh shader
    m_meshShader = std::make_unique<QOpenGLShaderProgram>();
    m_meshShader->addShaderFromSourceCode(QOpenGLShader::Vertex, MESH_VERTEX_SHADER);
    m_meshShader->addShaderFromSourceCode(QOpenGLShader::Fragment, MESH_FRAGMENT_SHADER);
    if(!m_meshShader->link()) {
        LOG_ERROR("SceneRenderer: Failed to link mesh shader: {}",
                  m_meshShader->log().toStdString());
    }

    m_mvpMatrixLoc = m_meshShader->uniformLocation("uMVPMatrix");
    m_modelMatrixLoc = m_meshShader->uniformLocation("uModelMatrix");
    m_normalMatrixLoc = m_meshShader->uniformLocation("uNormalMatrix");
    m_lightPosLoc = m_meshShader->uniformLocation("uLightPos");
    m_viewPosLoc = m_meshShader->uniformLocation("uViewPos");
    m_pointSizeLoc = m_meshShader->uniformLocation("uPointSize");
    m_useLightingLoc = m_meshShader->uniformLocation("uUseLighting");

    m_useOverrideColorLoc = m_meshShader->uniformLocation("uUseOverrideColor");
    m_overrideColorLoc = m_meshShader->uniformLocation("uOverrideColor");

    // Picking shader
    m_pickShader = std::make_unique<QOpenGLShaderProgram>();
    m_pickShader->addShaderFromSourceCode(QOpenGLShader::Vertex, PICK_VERTEX_SHADER);
    m_pickShader->addShaderFromSourceCode(QOpenGLShader::Fragment, PICK_FRAGMENT_SHADER);
    if(!m_pickShader->link()) {
        LOG_ERROR("SceneRenderer: Failed to link pick shader: {}",
                  m_pickShader->log().toStdString());
    }

    m_pickMvpMatrixLoc = m_pickShader->uniformLocation("uMVPMatrix");
    m_pickColorLoc = m_pickShader->uniformLocation("uPickColor");
    m_pickPointSizeLoc = m_pickShader->uniformLocation("uPointSize");

    // Picking-edge shader (thick lines via geometry shader)
    m_pickEdgeShader = std::make_unique<QOpenGLShaderProgram>();
    m_pickEdgeShader->addShaderFromSourceCode(QOpenGLShader::Vertex, PICK_VERTEX_SHADER);
    m_pickEdgeShader->addShaderFromSourceCode(QOpenGLShader::Geometry, PICK_EDGE_GEOMETRY_SHADER);
    m_pickEdgeShader->addShaderFromSourceCode(QOpenGLShader::Fragment, PICK_FRAGMENT_SHADER);
    if(!m_pickEdgeShader->link()) {
        LOG_ERROR("SceneRenderer: Failed to link pick edge shader: {}",
                  m_pickEdgeShader->log().toStdString());
    }

    m_pickEdgeMvpMatrixLoc = m_pickEdgeShader->uniformLocation("uMVPMatrix");
    m_pickEdgeColorLoc = m_pickEdgeShader->uniformLocation("uPickColor");
    m_pickEdgeViewportLoc = m_pickEdgeShader->uniformLocation("uViewport");
    m_pickEdgeThicknessLoc = m_pickEdgeShader->uniformLocation("uThickness");

    LOG_DEBUG("SceneRenderer: Shaders compiled and linked");
}

void SceneRenderer::uploadMeshData(const DocumentRenderData& render_data) {
    clearMeshBuffers();

    // Upload face meshes
    for(const auto& mesh : render_data.m_faceMeshes) {
        if(!mesh.isValid()) {
            continue;
        }

        MeshBuffers buffers;
        buffers.m_vao->create();
        buffers.m_vbo->create();
        if(mesh.isIndexed()) {
            buffers.m_ebo->create();
        }

        uploadMesh(mesh, *buffers.m_vao, *buffers.m_vbo, *buffers.m_ebo);

        buffers.m_vertexCount = static_cast<int>(mesh.vertexCount());
        buffers.m_indexCount = static_cast<int>(mesh.indexCount());
        buffers.m_primitiveType = mesh.m_primitiveType;
        buffers.m_entityType = mesh.m_entityType;
        buffers.m_entityUid = mesh.m_entityUid;

        m_faceMeshBuffers.push_back(std::move(buffers));
    }

    // Upload edge meshes
    for(const auto& mesh : render_data.m_edgeMeshes) {
        if(!mesh.isValid()) {
            continue;
        }

        MeshBuffers buffers;
        buffers.m_vao->create();
        buffers.m_vbo->create();
        if(mesh.isIndexed()) {
            buffers.m_ebo->create();
        }

        uploadMesh(mesh, *buffers.m_vao, *buffers.m_vbo, *buffers.m_ebo);

        buffers.m_vertexCount = static_cast<int>(mesh.vertexCount());
        buffers.m_indexCount = static_cast<int>(mesh.indexCount());
        buffers.m_primitiveType = mesh.m_primitiveType;
        buffers.m_entityType = mesh.m_entityType;
        buffers.m_entityUid = mesh.m_entityUid;

        m_edgeMeshBuffers.push_back(std::move(buffers));
    }

    // Upload vertex meshes
    for(const auto& mesh : render_data.m_vertexMeshes) {
        if(!mesh.isValid()) {
            continue;
        }

        MeshBuffers buffers;
        buffers.m_vao->create();
        buffers.m_vbo->create();

        uploadMesh(mesh, *buffers.m_vao, *buffers.m_vbo, *buffers.m_ebo);

        buffers.m_vertexCount = static_cast<int>(mesh.vertexCount());
        buffers.m_primitiveType = mesh.m_primitiveType;
        buffers.m_entityType = mesh.m_entityType;
        buffers.m_entityUid = mesh.m_entityUid;

        m_vertexMeshBuffers.push_back(std::move(buffers));
    }

    LOG_DEBUG("SceneRenderer: Uploaded {} face meshes, {} edge meshes, {} vertex meshes",
              m_faceMeshBuffers.size(), m_edgeMeshBuffers.size(), m_vertexMeshBuffers.size());
}

void SceneRenderer::uploadMesh(const RenderMesh& mesh,
                               QOpenGLVertexArrayObject& vao,
                               QOpenGLBuffer& vbo,
                               QOpenGLBuffer& ebo) {
    vao.bind();
    vbo.bind();

    // Upload vertex data
    vbo.allocate(mesh.m_vertices.data(),
                 static_cast<int>(mesh.m_vertices.size() * sizeof(RenderVertex)));

    // Setup vertex attributes
    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                          reinterpret_cast<void*>(offsetof(RenderVertex, m_position)));

    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                          reinterpret_cast<void*>(offsetof(RenderVertex, m_normal)));

    // Color (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                          reinterpret_cast<void*>(offsetof(RenderVertex, m_color)));

    // Upload index data if present
    if(mesh.isIndexed() && ebo.isCreated()) {
        ebo.bind();
        ebo.allocate(mesh.m_indices.data(),
                     static_cast<int>(mesh.m_indices.size() * sizeof(uint32_t)));
    }

    vao.release();
    vbo.release();
    if(ebo.isCreated()) {
        ebo.release();
    }
}

void SceneRenderer::clearMeshBuffers() {
    m_faceMeshBuffers.clear();
    m_edgeMeshBuffers.clear();
    m_vertexMeshBuffers.clear();
}

void SceneRenderer::render(const QVector3D& camera_pos,
                           const QMatrix4x4& view_matrix,
                           const QMatrix4x4& projection_matrix) {
    if(!m_initialized) {
        LOG_WARN("SceneRenderer: render() called before initialize()");
        return;
    }

    // Clear background
    glClearColor(0.15f, 0.15f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calculate matrices
    const QMatrix4x4 model; // Identity
    const QMatrix4x4 mvp = projection_matrix * view_matrix * model;

    // Render meshes
    renderMeshes(mvp, model, camera_pos);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void SceneRenderer::renderMeshes(const QMatrix4x4& mvp,
                                 const QMatrix4x4& model_matrix,
                                 const QVector3D& camera_pos) {
    if(!m_meshShader || !m_meshShader->isLinked()) {
        return;
    }

    const QMatrix3x3 normal_matrix = model_matrix.normalMatrix();

    m_meshShader->bind();
    m_meshShader->setUniformValue(m_mvpMatrixLoc, mvp);
    m_meshShader->setUniformValue(m_modelMatrixLoc, model_matrix);
    m_meshShader->setUniformValue(m_normalMatrixLoc, normal_matrix);
    m_meshShader->setUniformValue(m_lightPosLoc, camera_pos);
    m_meshShader->setUniformValue(m_viewPosLoc, camera_pos);
    m_meshShader->setUniformValue(m_useLightingLoc, 1);
    m_meshShader->setUniformValue(m_useOverrideColorLoc, 0);
    m_meshShader->setUniformValue(m_overrideColorLoc, QVector4D(1.0f, 1.0f, 0.0f, 1.0f));

    auto get_primitive_type = [](RenderPrimitiveType type) -> GLenum {
        switch(type) {
        case RenderPrimitiveType::Points:
            return GL_POINTS;
        case RenderPrimitiveType::Lines:
            return GL_LINES;
        case RenderPrimitiveType::LineStrip:
            return GL_LINE_STRIP;
        case RenderPrimitiveType::Triangles:
            return GL_TRIANGLES;
        case RenderPrimitiveType::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case RenderPrimitiveType::TriangleFan:
            return GL_TRIANGLE_FAN;
        default:
            return GL_TRIANGLES;
        }
    };

    // Enable shader-controlled point size
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Set default point size for faces/edges
    m_meshShader->setUniformValue(m_pointSizeLoc, 1.0f);

    // Render face meshes
    glDepthFunc(GL_LESS);
    m_meshShader->setUniformValue(m_useLightingLoc, 1);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    for(auto& buffers : m_faceMeshBuffers) {
        const bool is_highlight =
            (m_highlightType != Geometry::EntityType::None) &&
            (buffers.m_entityType == m_highlightType) &&
            ((buffers.m_entityUid & 0xFFFFFFu) == (m_highlightUid & 0xFFFFFFu));
        if(is_highlight) {
            m_meshShader->setUniformValue(m_useOverrideColorLoc, 1);
            m_meshShader->setUniformValue(m_overrideColorLoc, QVector4D(0.15f, 0.65f, 1.0f, 1.0f));
        } else {
            m_meshShader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        buffers.m_vao->bind();
        if(buffers.m_indexCount > 0) {
            buffers.m_ebo->bind();
            glDrawElements(get_primitive_type(buffers.m_primitiveType), buffers.m_indexCount,
                           GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(get_primitive_type(buffers.m_primitiveType), 0, buffers.m_vertexCount);
        }
        buffers.m_vao->release();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Render edge meshes (with line width)
    // Allow edges at the same depth to pass (and stay visible on faces).
    glDepthFunc(GL_LEQUAL);
    m_meshShader->setUniformValue(m_useLightingLoc, 0);
    glLineWidth(2.0f);
    for(auto& buffers : m_edgeMeshBuffers) {
        const bool is_highlight =
            (m_highlightType != Geometry::EntityType::None) &&
            (buffers.m_entityType == m_highlightType) &&
            ((buffers.m_entityUid & 0xFFFFFFu) == (m_highlightUid & 0xFFFFFFu));
        if(is_highlight) {
            m_meshShader->setUniformValue(m_useOverrideColorLoc, 1);
            m_meshShader->setUniformValue(m_overrideColorLoc, QVector4D(1.0f, 0.3f, 0.1f, 1.0f));
        } else {
            m_meshShader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        buffers.m_vao->bind();
        if(buffers.m_indexCount > 0) {
            buffers.m_ebo->bind();
            glDrawElements(get_primitive_type(buffers.m_primitiveType), buffers.m_indexCount,
                           GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(get_primitive_type(buffers.m_primitiveType), 0, buffers.m_vertexCount);
        }
        buffers.m_vao->release();
    }

    // Render vertex meshes (with larger point size)
    // Allow points at the same depth to pass (and stay visible on faces).
    glDepthFunc(GL_LEQUAL);
    m_meshShader->setUniformValue(m_useLightingLoc, 0);
    for(auto& buffers : m_vertexMeshBuffers) {
        const bool is_highlight =
            (m_highlightType != Geometry::EntityType::None) &&
            (buffers.m_entityType == m_highlightType) &&
            ((buffers.m_entityUid & 0xFFFFFFu) == (m_highlightUid & 0xFFFFFFu));
        if(is_highlight) {
            m_meshShader->setUniformValue(m_pointSizeLoc, 9.0f);
            m_meshShader->setUniformValue(m_useOverrideColorLoc, 1);
            m_meshShader->setUniformValue(m_overrideColorLoc, QVector4D(0.0f, 1.0f, 0.35f, 1.0f));
        } else {
            m_meshShader->setUniformValue(m_pointSizeLoc, 6.0f);
            m_meshShader->setUniformValue(m_useOverrideColorLoc, 0);
        }

        buffers.m_vao->bind();
        glDrawArrays(GL_POINTS, 0, buffers.m_vertexCount);
        buffers.m_vao->release();
    }

    glDepthFunc(GL_LESS);

    m_meshShader->release();
}

namespace {
[[nodiscard]] QVector4D encodePickColor(OpenGeoLab::Geometry::EntityType type,
                                        OpenGeoLab::Geometry::EntityUID uid) {
    const uint32_t uid24 = static_cast<uint32_t>(uid & 0xFFFFFFu);
    const uint8_t r = static_cast<uint8_t>((uid24 >> 0) & 0xFFu);
    const uint8_t g = static_cast<uint8_t>((uid24 >> 8) & 0xFFu);
    const uint8_t b = static_cast<uint8_t>((uid24 >> 16) & 0xFFu);
    const uint8_t a = static_cast<uint8_t>(type);
    return QVector4D(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
} // namespace

void SceneRenderer::renderPicking(const QMatrix4x4& view_matrix,
                                  const QMatrix4x4& projection_matrix) {
    if(!m_initialized || !m_pickShader || !m_pickShader->isLinked() || !m_pickEdgeShader ||
       !m_pickEdgeShader->isLinked()) {
        return;
    }

    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDepthFunc(GL_LESS);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const QMatrix4x4 model; // Identity
    const QMatrix4x4 mvp = projection_matrix * view_matrix * model;

    // Faces + vertices (regular pick shader)
    m_pickShader->bind();
    m_pickShader->setUniformValue(m_pickMvpMatrixLoc, mvp);

    // Faces
    m_pickShader->setUniformValue(m_pickPointSizeLoc, 1.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    for(auto& buffers : m_faceMeshBuffers) {
        m_pickShader->setUniformValue(m_pickColorLoc,
                                      encodePickColor(buffers.m_entityType, buffers.m_entityUid));
        buffers.m_vao->bind();
        if(buffers.m_indexCount > 0) {
            buffers.m_ebo->bind();
            glDrawElements(GL_TRIANGLES, buffers.m_indexCount, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, buffers.m_vertexCount);
        }
        buffers.m_vao->release();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Allow edge/vertex at the same depth to win over faces
    glDepthFunc(GL_LEQUAL);

    // Edges
    m_pickShader->release();

    m_pickEdgeShader->bind();
    m_pickEdgeShader->setUniformValue(m_pickEdgeMvpMatrixLoc, mvp);
    m_pickEdgeShader->setUniformValue(m_pickEdgeViewportLoc,
                                      QVector2D(static_cast<float>(m_viewportSize.width()),
                                                static_cast<float>(m_viewportSize.height())));
    m_pickEdgeShader->setUniformValue(m_pickEdgeThicknessLoc, 10.0f);
    // uPointSize exists in the shared vertex shader; keep it valid.
    m_pickEdgeShader->setUniformValue(m_pickPointSizeLoc, 1.0f);

    for(auto& buffers : m_edgeMeshBuffers) {
        m_pickEdgeShader->setUniformValue(
            m_pickEdgeColorLoc, encodePickColor(buffers.m_entityType, buffers.m_entityUid));
        buffers.m_vao->bind();
        glDrawArrays(GL_LINE_STRIP, 0, buffers.m_vertexCount);
        buffers.m_vao->release();
    }

    m_pickEdgeShader->release();

    // Vertices (regular pick shader)
    m_pickShader->bind();
    m_pickShader->setUniformValue(m_pickMvpMatrixLoc, mvp);

    // Vertices
    m_pickShader->setUniformValue(m_pickPointSizeLoc, 16.0f);
    for(auto& buffers : m_vertexMeshBuffers) {
        m_pickShader->setUniformValue(m_pickColorLoc,
                                      encodePickColor(buffers.m_entityType, buffers.m_entityUid));
        buffers.m_vao->bind();
        glDrawArrays(GL_POINTS, 0, buffers.m_vertexCount);
        buffers.m_vao->release();
    }

    m_pickShader->release();

    glDepthFunc(GL_LESS);
}

void SceneRenderer::setHighlightedEntity(Geometry::EntityType type, Geometry::EntityUID uid) {
    if(type == Geometry::EntityType::None || uid == Geometry::INVALID_ENTITY_UID) {
        m_highlightType = Geometry::EntityType::None;
        m_highlightUid = Geometry::INVALID_ENTITY_UID;
        return;
    }

    m_highlightType = type;
    m_highlightUid = static_cast<Geometry::EntityUID>(uid & 0xFFFFFFu);
}

void SceneRenderer::cleanup() {
    clearMeshBuffers();

    m_meshShader.reset();
    m_pickShader.reset();
    m_pickEdgeShader.reset();
    m_initialized = false;

    LOG_DEBUG("SceneRenderer: Cleanup complete");
}

} // namespace OpenGeoLab::Render
