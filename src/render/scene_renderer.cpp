/**
 * @file scene_renderer.cpp
 * @brief Implementation of SceneRenderer for OpenGL geometry visualization
 */

#include "render/scene_renderer.hpp"
#include "util/logger.hpp"

#include <QOpenGLFramebufferObject>

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
    vNormal = normalize(uNormalMatrix * aNormal);
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
uniform int uHighlightState;  // 0=None, 1=Preview, 2=Selected
uniform vec4 uHighlightColor; // Override color for highlights

out vec4 fragColor;

void main() {
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);

    // Diffuse
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    // Specular
    float specularStrength = 0.3;
    vec3 viewDir = normalize(uViewPos - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0);

    // Apply highlight color override if highlighted
    vec4 baseColor = vColor;
    if (uHighlightState > 0) {
        baseColor = uHighlightColor;
    }

    vec3 result = (ambient + diffuse + specular) * baseColor.rgb;
    fragColor = vec4(result, baseColor.a);
}
)";

// Picking shader - outputs entity ID as color
const char* const PICKING_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uMVPMatrix;
uniform float uPointSize;

void main() {
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
}
)";

const char* const PICKING_FRAGMENT_SHADER = R"(
#version 330 core
uniform vec4 uEntityColor;

out vec4 fragColor;

void main() {
    fragColor = uEntityColor;
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
    m_highlightStateLoc = m_meshShader->uniformLocation("uHighlightState");
    m_highlightColorLoc = m_meshShader->uniformLocation("uHighlightColor");

    // Picking shader
    m_pickingShader = std::make_unique<QOpenGLShaderProgram>();
    m_pickingShader->addShaderFromSourceCode(QOpenGLShader::Vertex, PICKING_VERTEX_SHADER);
    m_pickingShader->addShaderFromSourceCode(QOpenGLShader::Fragment, PICKING_FRAGMENT_SHADER);
    if(!m_pickingShader->link()) {
        LOG_ERROR("SceneRenderer: Failed to link picking shader: {}",
                  m_pickingShader->log().toStdString());
    }

    m_pickingMvpLoc = m_pickingShader->uniformLocation("uMVPMatrix");
    m_pickingColorLoc = m_pickingShader->uniformLocation("uEntityColor");
    m_pickingPointSizeLoc = m_pickingShader->uniformLocation("uPointSize");

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
        buffers.m_highlightState = mesh.m_highlightState;
        buffers.m_entityId = mesh.m_entityId;

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
        buffers.m_highlightState = mesh.m_highlightState;
        buffers.m_entityId = mesh.m_entityId;

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
        buffers.m_highlightState = mesh.m_highlightState;
        buffers.m_entityId = mesh.m_entityId;

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

    // Highlight colors: Preview=cyan, Selected=orange
    const QVector4D previewColor(0.0f, 0.8f, 1.0f, 1.0f);
    const QVector4D selectedColor(1.0f, 0.5f, 0.0f, 1.0f);

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

    auto setHighlightUniforms = [this, &previewColor, &selectedColor](HighlightState state) {
        int stateInt = static_cast<int>(state);
        m_meshShader->setUniformValue(m_highlightStateLoc, stateInt);
        if(state == HighlightState::Preview) {
            m_meshShader->setUniformValue(m_highlightColorLoc, previewColor);
        } else if(state == HighlightState::Selected) {
            m_meshShader->setUniformValue(m_highlightColorLoc, selectedColor);
        }
    };

    // Enable shader-controlled point size
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Set default point size for faces/edges
    m_meshShader->setUniformValue(m_pointSizeLoc, 1.0f);

    // Render face meshes
    for(auto& buffers : m_faceMeshBuffers) {
        setHighlightUniforms(buffers.m_highlightState);
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

    // Render edge meshes (with line width)
    glLineWidth(2.0f);
    for(auto& buffers : m_edgeMeshBuffers) {
        setHighlightUniforms(buffers.m_highlightState);
        // Use thicker lines for highlighted edges
        if(buffers.m_highlightState != HighlightState::None) {
            glLineWidth(4.0f);
        } else {
            glLineWidth(2.0f);
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
    for(auto& buffers : m_vertexMeshBuffers) {
        setHighlightUniforms(buffers.m_highlightState);
        // Use larger points for highlighted vertices
        float pointSize = (buffers.m_highlightState != HighlightState::None) ? 8.0f : 5.0f;
        m_meshShader->setUniformValue(m_pointSizeLoc, pointSize);
        buffers.m_vao->bind();
        glDrawArrays(GL_POINTS, 0, buffers.m_vertexCount);
        buffers.m_vao->release();
    }

    m_meshShader->release();
}

// =============================================================================
// Entity ID Picking
// =============================================================================

QVector4D SceneRenderer::encodeEntityIdToColor(Geometry::EntityId entity_id) {
    // Encode 32-bit entity ID into RGBA (using first 32 bits)
    // R = bits 0-7, G = bits 8-15, B = bits 16-23, A = bits 24-31
    const uint8_t r = static_cast<uint8_t>((entity_id >> 0) & 0xFF);
    const uint8_t g = static_cast<uint8_t>((entity_id >> 8) & 0xFF);
    const uint8_t b = static_cast<uint8_t>((entity_id >> 16) & 0xFF);
    const uint8_t a = static_cast<uint8_t>((entity_id >> 24) & 0xFF);

    return QVector4D(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

Geometry::EntityId
SceneRenderer::decodeEntityIdFromColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // Background is black (0,0,0,0) which corresponds to INVALID_ENTITY_ID (0)
    Geometry::EntityId id =
        static_cast<Geometry::EntityId>(r) | (static_cast<Geometry::EntityId>(g) << 8) |
        (static_cast<Geometry::EntityId>(b) << 16) | (static_cast<Geometry::EntityId>(a) << 24);
    return id;
}

void SceneRenderer::renderForPicking(const QMatrix4x4& view_matrix,
                                     const QMatrix4x4& projection_matrix,
                                     Geometry::SelectionMode selection_mode) {
    if(!m_initialized) {
        LOG_WARN("SceneRenderer: renderForPicking() called before initialize()");
        return;
    }

    // Clear with black background (entity ID 0 = invalid)
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND); // No blending for picking

    const QMatrix4x4 model; // Identity
    const QMatrix4x4 mvp = projection_matrix * view_matrix * model;

    renderMeshesForPicking(mvp, selection_mode);

    glDisable(GL_DEPTH_TEST);
}

void SceneRenderer::renderMeshesForPicking(const QMatrix4x4& mvp,
                                           Geometry::SelectionMode selection_mode) {
    if(!m_pickingShader || !m_pickingShader->isLinked()) {
        return;
    }

    m_pickingShader->bind();
    m_pickingShader->setUniformValue(m_pickingMvpLoc, mvp);

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

    auto shouldRenderEntityType = [selection_mode](Geometry::EntityType type) -> bool {
        switch(selection_mode) {
        case Geometry::SelectionMode::None:
            return false;
        case Geometry::SelectionMode::Vertex:
            return type == Geometry::EntityType::Vertex;
        case Geometry::SelectionMode::Edge:
            return type == Geometry::EntityType::Edge;
        case Geometry::SelectionMode::Face:
            return type == Geometry::EntityType::Face;
        case Geometry::SelectionMode::Solid:
            return type == Geometry::EntityType::Solid;
        case Geometry::SelectionMode::Part:
            return type == Geometry::EntityType::Part;
        case Geometry::SelectionMode::Multi:
            return true; // All types
        default:
            return true;
        }
    };

    glEnable(GL_PROGRAM_POINT_SIZE);
    m_pickingShader->setUniformValue(m_pickingPointSizeLoc, 1.0f);

    // Render face meshes for picking (if face or multi mode)
    if(shouldRenderEntityType(Geometry::EntityType::Face)) {
        for(auto& buffers : m_faceMeshBuffers) {
            QVector4D entityColor = encodeEntityIdToColor(buffers.m_entityId);
            m_pickingShader->setUniformValue(m_pickingColorLoc, entityColor);

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
    }

    // Render edge meshes for picking (if edge or multi mode)
    if(shouldRenderEntityType(Geometry::EntityType::Edge)) {
        glLineWidth(4.0f); // Thicker lines for easier picking
        for(auto& buffers : m_edgeMeshBuffers) {
            QVector4D entityColor = encodeEntityIdToColor(buffers.m_entityId);
            m_pickingShader->setUniformValue(m_pickingColorLoc, entityColor);

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
    }

    // Render vertex meshes for picking (if vertex or multi mode)
    if(shouldRenderEntityType(Geometry::EntityType::Vertex)) {
        m_pickingShader->setUniformValue(m_pickingPointSizeLoc, 10.0f); // Larger points for picking
        for(auto& buffers : m_vertexMeshBuffers) {
            QVector4D entityColor = encodeEntityIdToColor(buffers.m_entityId);
            m_pickingShader->setUniformValue(m_pickingColorLoc, entityColor);

            buffers.m_vao->bind();
            glDrawArrays(GL_POINTS, 0, buffers.m_vertexCount);
            buffers.m_vao->release();
        }
    }

    m_pickingShader->release();
}

void SceneRenderer::cleanup() {
    clearMeshBuffers();

    m_meshShader.reset();
    m_pickingShader.reset();
    m_initialized = false;

    LOG_DEBUG("SceneRenderer: Cleanup complete");
}

} // namespace OpenGeoLab::Render
