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
uniform bool uHighlighted;

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vColor;
flat out int vHighlighted;

void main() {
    vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    vHighlighted = uHighlighted ? 1 : 0;
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    gl_PointSize = uHighlighted ? uPointSize * 1.5 : uPointSize;
}
)";

const char* const MESH_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vColor;
flat in int vHighlighted;

uniform vec3 uLightPos;
uniform vec3 uViewPos;

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

    vec3 baseColor = vColor.rgb;
    
    // Apply highlight effect - brighter orange/yellow tint
    if (vHighlighted == 1) {
        baseColor = mix(baseColor, vec3(1.0, 0.65, 0.15), 0.75);
    }

    vec3 result = (ambient + diffuse + specular) * baseColor;
    fragColor = vec4(result, vColor.a);
}
)";

/// ID buffer vertex shader for entity picking
const char* const ID_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uMVPMatrix;
uniform float uPointSize;

void main() {
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
}
)";

/// ID buffer fragment shader for entity picking
const char* const ID_FRAGMENT_SHADER = R"(
#version 330 core
uniform vec4 uIdColor;

out vec4 fragColor;

void main() {
    fragColor = uIdColor;
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
    m_highlightedLoc = m_meshShader->uniformLocation("uHighlighted");

    // ID shader for picking
    m_idShader = std::make_unique<QOpenGLShaderProgram>();
    m_idShader->addShaderFromSourceCode(QOpenGLShader::Vertex, ID_VERTEX_SHADER);
    m_idShader->addShaderFromSourceCode(QOpenGLShader::Fragment, ID_FRAGMENT_SHADER);
    if(!m_idShader->link()) {
        LOG_ERROR("SceneRenderer: Failed to link ID shader: {}", m_idShader->log().toStdString());
    }
    m_idMvpMatrixLoc = m_idShader->uniformLocation("uMVPMatrix");
    m_idColorLoc = m_idShader->uniformLocation("uIdColor");
    m_idPointSizeLoc = m_idShader->uniformLocation("uPointSize");

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
        buffers.m_entityUid = mesh.m_entityUid;
        buffers.m_entityType = mesh.m_entityType;

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
        buffers.m_entityUid = mesh.m_entityUid;
        buffers.m_entityType = mesh.m_entityType;

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
        buffers.m_entityUid = mesh.m_entityUid;
        buffers.m_entityType = mesh.m_entityType;

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

    // Helper lambda to render buffers with highlight support
    auto render_with_highlight = [&](std::vector<MeshBuffers>& buffers, float point_size) {
        m_meshShader->setUniformValue(m_pointSizeLoc, point_size);
        for(auto& buf : buffers) {
            // Check if this entity should be highlighted
            const bool is_highlighted = (m_highlightedEntityUid != Geometry::INVALID_ENTITY_UID &&
                                         buf.m_entityUid == m_highlightedEntityUid);
            m_meshShader->setUniformValue(m_highlightedLoc, is_highlighted);

            buf.m_vao->bind();
            if(buf.m_indexCount > 0) {
                buf.m_ebo->bind();
                glDrawElements(get_primitive_type(buf.m_primitiveType), buf.m_indexCount,
                               GL_UNSIGNED_INT, nullptr);
            } else {
                glDrawArrays(get_primitive_type(buf.m_primitiveType), 0, buf.m_vertexCount);
            }
            buf.m_vao->release();
        }
    };

    // Render face meshes
    render_with_highlight(m_faceMeshBuffers, 1.0f);

    // Render edge meshes (with line width)
    m_meshShader->setUniformValue(m_pointSizeLoc, 1.0f);
    for(auto& buf : m_edgeMeshBuffers) {
        const bool is_highlighted = (m_highlightedEntityUid != Geometry::INVALID_ENTITY_UID &&
                                     buf.m_entityUid == m_highlightedEntityUid);
        m_meshShader->setUniformValue(m_highlightedLoc, is_highlighted);
        glLineWidth(is_highlighted ? 4.0f : 2.0f);

        buf.m_vao->bind();
        if(buf.m_indexCount > 0) {
            buf.m_ebo->bind();
            glDrawElements(get_primitive_type(buf.m_primitiveType), buf.m_indexCount,
                           GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(get_primitive_type(buf.m_primitiveType), 0, buf.m_vertexCount);
        }
        buf.m_vao->release();
    }

    // Render vertex meshes (with larger point size)
    render_with_highlight(m_vertexMeshBuffers, 6.0f);

    m_meshShader->release();
}

void SceneRenderer::cleanup() {
    clearMeshBuffers();

    m_meshShader.reset();
    m_idShader.reset();
    m_initialized = false;

    LOG_DEBUG("SceneRenderer: Cleanup complete");
}

void SceneRenderer::setHighlightedEntityUid(Geometry::EntityUID uid) {
    m_highlightedEntityUid = uid;
}

Geometry::EntityUID SceneRenderer::highlightedEntityUid() const { return m_highlightedEntityUid; }

// =============================================================================
// ID Buffer Rendering for Picking
// =============================================================================

void SceneRenderer::encodeEntityId(Geometry::EntityUID entity_uid, // NOLINT
                                   Geometry::EntityType entity_type,
                                   uint8_t& r,
                                   uint8_t& g,
                                   uint8_t& b,
                                   uint8_t& a) {
    // Encoding scheme:
    // R: Low 8 bits of entityUid
    // G: Mid 8 bits of entityUid
    // B: High 8 bits of entityUid (limited to 24-bit UIDs)
    // A: EntityType enum value
    r = static_cast<uint8_t>(entity_uid & 0xFF);
    g = static_cast<uint8_t>((entity_uid >> 8) & 0xFF);
    b = static_cast<uint8_t>((entity_uid >> 16) & 0xFF);
    a = static_cast<uint8_t>(entity_type);
}
bool SceneRenderer::decodeEntityId(uint8_t r, // NOLINT
                                   uint8_t g,
                                   uint8_t b,
                                   uint8_t a,
                                   Geometry::EntityUID& entity_uid,
                                   Geometry::EntityType& entity_type) {
    // Background is encoded as (0, 0, 0, 0)
    if(r == 0 && g == 0 && b == 0 && a == 0) {
        return false;
    }

    entity_uid = static_cast<Geometry::EntityUID>(r) | (static_cast<Geometry::EntityUID>(g) << 8) |
                 (static_cast<Geometry::EntityUID>(b) << 16);
    entity_type = static_cast<Geometry::EntityType>(a);
    return true;
}

void SceneRenderer::renderIdBuffer(const QMatrix4x4& mvp) {
    if(!m_idShader || !m_idShader->isLinked()) {
        return;
    }

    m_idShader->bind();
    m_idShader->setUniformValue(m_idMvpMatrixLoc, mvp);

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

    auto render_buffers_with_id = [&](std::vector<MeshBuffers>& buffers, float point_size) {
        m_idShader->setUniformValue(m_idPointSizeLoc, point_size);
        for(auto& buf : buffers) {
            uint8_t r, g, b, a;
            encodeEntityId(buf.m_entityUid, buf.m_entityType, r, g, b, a);
            m_idShader->setUniformValue(m_idColorLoc,
                                        QVector4D(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f));

            buf.m_vao->bind();
            if(buf.m_indexCount > 0) {
                buf.m_ebo->bind();
                glDrawElements(get_primitive_type(buf.m_primitiveType), buf.m_indexCount,
                               GL_UNSIGNED_INT, nullptr);
            } else {
                glDrawArrays(get_primitive_type(buf.m_primitiveType), 0, buf.m_vertexCount);
            }
            buf.m_vao->release();
        }
    };

    glEnable(GL_PROGRAM_POINT_SIZE);

    // Render order is important for picking priority:
    // 1. Faces first (lowest priority)
    // 2. Edges second (medium priority)
    // 3. Vertices last (highest priority)
    // This ensures vertices and edges are drawn on top of faces in the ID buffer

    // Render faces
    render_buffers_with_id(m_faceMeshBuffers, 1.0f);

    // Render edges with increased line width for better picking
    // Using larger line width for easier picking of thin edges
    glLineWidth(6.0f);
    render_buffers_with_id(m_edgeMeshBuffers, 1.0f);

    // Render vertices with large point size for picking
    // Larger point size makes vertices easier to pick
    render_buffers_with_id(m_vertexMeshBuffers, 14.0f);

    m_idShader->release();
}

PickPixelResult SceneRenderer::pickAtPixel(int x,
                                           int y,
                                           const QMatrix4x4& view_matrix,
                                           const QMatrix4x4& projection_matrix) {
    PickPixelResult result;

    if(!m_initialized) {
        LOG_WARN("SceneRenderer: pickAtPixel() called before initialize()");
        return result;
    }

    if(x < 0 || y < 0 || x >= m_viewportSize.width() || y >= m_viewportSize.height()) {
        return result;
    }

    // Create temporary FBO for ID buffer rendering
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(0); // No MSAA for picking - need exact colors
    QOpenGLFramebufferObject fbo(m_viewportSize, format);

    if(!fbo.bind()) {
        LOG_ERROR("SceneRenderer: Failed to bind picking FBO");
        return result;
    }

    // Clear with background color (0, 0, 0, 0)
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND); // Disable blending for ID buffer

    // Calculate matrices
    const QMatrix4x4 model; // Identity
    const QMatrix4x4 mvp = projection_matrix * view_matrix * model;

    // Render ID buffer
    renderIdBuffer(mvp);

    // Read pixel at pick position
    // Note: OpenGL Y is inverted compared to screen coordinates
    int gl_y = m_viewportSize.height() - y - 1;

    uint8_t pixel[4] = {0, 0, 0, 0};
    glReadPixels(x, gl_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    fbo.release();

    // Decode picked entity
    Geometry::EntityUID uid;
    Geometry::EntityType type;
    if(decodeEntityId(pixel[0], pixel[1], pixel[2], pixel[3], uid, type)) {
        result.m_valid = true;
        result.m_entityUid = uid;
        result.m_entityType = type;
        LOG_TRACE("SceneRenderer: Picked entity UID={}, type={}", uid, static_cast<int>(type));
    }

    return result;
}

} // namespace OpenGeoLab::Render
