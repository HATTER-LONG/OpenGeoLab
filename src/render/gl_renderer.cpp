/**
 * @file gl_renderer.cpp
 * @brief Implementation of OpenGL renderer
 */

#include "render/gl_renderer.hpp"
#include "util/logger.hpp"

#include <QMatrix4x4>
#include <QVector3D>

#include <cmath>

namespace OpenGeoLab::Render {

// Face vertex shader
static const char* FACE_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aEntityId;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec4 vColor;
out vec3 vPosition;
flat out uint vEntityId;

void main() {
    vec4 viewPos = uModelView * vec4(aPosition, 1.0);
    vPosition = viewPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    vEntityId = uint(aEntityId);
    gl_Position = uProjection * viewPos;
}
)";

// Face fragment shader with Phong lighting
static const char* FACE_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 vNormal;
in vec4 vColor;
in vec3 vPosition;
flat in uint vEntityId;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform bool uSelected;
uniform bool uHighlighted;
uniform vec4 uSelectedColor;
uniform vec4 uHighlightColor;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    // Ambient
    float ambient = 0.3;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    float diffuse = diff * 0.6;

    // Specular
    vec3 viewDir = normalize(-vPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    float specular = spec * 0.3;

    vec3 baseColor = vColor.rgb;

    // Apply selection/highlight tint
    if (uSelected) {
        baseColor = mix(baseColor, uSelectedColor.rgb, 0.5);
    } else if (uHighlighted) {
        baseColor = mix(baseColor, uHighlightColor.rgb, 0.3);
    }

    vec3 result = (ambient + diffuse + specular) * baseColor * uLightColor;
    fragColor = vec4(result, vColor.a);
}
)";

// Edge vertex shader
static const char* EDGE_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aEntityId;

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
}
)";

// Edge fragment shader
static const char* EDGE_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vColor;
out vec4 fragColor;

void main() {
    fragColor = vColor;
}
)";

GLRenderer::GLRenderer() = default;

GLRenderer::~GLRenderer() {
    if(m_initialized) {
        cleanup();
    }
}

bool GLRenderer::initialize() {
    if(m_initialized) {
        return true;
    }

    initializeOpenGLFunctions();

    if(!compileShaders()) {
        LOG_ERROR("Failed to compile shaders");
        return false;
    }

    // Set default lighting
    m_mainLight.position = {1.0f, 1.0f, 1.0f};
    m_mainLight.color = {1.0f, 1.0f, 1.0f};
    m_mainLight.intensity = 1.0f;
    m_mainLight.directional = true;

    m_initialized = true;
    LOG_INFO("OpenGL renderer initialized");
    return true;
}

void GLRenderer::cleanup() {
    clearMeshes();

    m_faceShader.reset();
    m_edgeShader.reset();
    m_pickShader.reset();

    m_initialized = false;
    LOG_INFO("OpenGL renderer cleaned up");
}

bool GLRenderer::compileShaders() {
    // Face shader
    m_faceShader = std::make_unique<QOpenGLShaderProgram>();
    if(!m_faceShader->addShaderFromSourceCode(QOpenGLShader::Vertex, FACE_VERTEX_SHADER)) {
        LOG_ERROR("Face vertex shader compilation failed: {}", m_faceShader->log().toStdString());
        return false;
    }
    if(!m_faceShader->addShaderFromSourceCode(QOpenGLShader::Fragment, FACE_FRAGMENT_SHADER)) {
        LOG_ERROR("Face fragment shader compilation failed: {}", m_faceShader->log().toStdString());
        return false;
    }
    if(!m_faceShader->link()) {
        LOG_ERROR("Face shader linking failed: {}", m_faceShader->log().toStdString());
        return false;
    }

    // Get face shader uniform locations
    m_faceModelViewLoc = m_faceShader->uniformLocation("uModelView");
    m_faceProjectionLoc = m_faceShader->uniformLocation("uProjection");
    m_faceNormalMatrixLoc = m_faceShader->uniformLocation("uNormalMatrix");
    m_faceLightDirLoc = m_faceShader->uniformLocation("uLightDir");
    m_faceLightColorLoc = m_faceShader->uniformLocation("uLightColor");
    m_faceSelectedLoc = m_faceShader->uniformLocation("uSelected");
    m_faceHighlightedLoc = m_faceShader->uniformLocation("uHighlighted");
    m_faceSelectedColorLoc = m_faceShader->uniformLocation("uSelectedColor");
    m_faceHighlightColorLoc = m_faceShader->uniformLocation("uHighlightColor");

    // Edge shader
    m_edgeShader = std::make_unique<QOpenGLShaderProgram>();
    if(!m_edgeShader->addShaderFromSourceCode(QOpenGLShader::Vertex, EDGE_VERTEX_SHADER)) {
        LOG_ERROR("Edge vertex shader compilation failed: {}", m_edgeShader->log().toStdString());
        return false;
    }
    if(!m_edgeShader->addShaderFromSourceCode(QOpenGLShader::Fragment, EDGE_FRAGMENT_SHADER)) {
        LOG_ERROR("Edge fragment shader compilation failed: {}", m_edgeShader->log().toStdString());
        return false;
    }
    if(!m_edgeShader->link()) {
        LOG_ERROR("Edge shader linking failed: {}", m_edgeShader->log().toStdString());
        return false;
    }

    m_edgeModelViewLoc = m_edgeShader->uniformLocation("uModelView");
    m_edgeProjectionLoc = m_edgeShader->uniformLocation("uProjection");

    return true;
}

void GLRenderer::setScene(const RenderScene& scene) {
    clearMeshes();

    for(const auto& mesh : scene.meshes) {
        updateMesh(mesh);
    }
}

void GLRenderer::updateMesh(const RenderMeshPtr& mesh) {
    if(!mesh || !m_initialized) {
        return;
    }

    // Find or create buffers
    MeshBuffers* buffers = findMeshBuffers(mesh->entityId);
    if(!buffers) {
        m_meshBuffers.push_back(std::make_unique<MeshBuffers>());
        buffers = m_meshBuffers.back().get();
        buffers->entityId = mesh->entityId;
    }

    uploadMeshToGPU(mesh, *buffers);
}

void GLRenderer::removeMesh(Geometry::EntityId entityId) {
    auto iter = std::remove_if(
        m_meshBuffers.begin(), m_meshBuffers.end(),
        [entityId](const std::unique_ptr<MeshBuffers>& buf) { return buf->entityId == entityId; });
    m_meshBuffers.erase(iter, m_meshBuffers.end());
}

void GLRenderer::clearMeshes() { m_meshBuffers.clear(); }

void GLRenderer::setCamera(const Camera& camera) { m_camera = camera; }

void GLRenderer::setDisplaySettings(const DisplaySettings& settings) {
    m_displaySettings = settings;
}

void GLRenderer::render(int width, int height) {
    if(!m_initialized || width <= 0 || height <= 0) {
        return;
    }

    // Setup viewport and clear
    glViewport(0, 0, width, height);
    glClearColor(m_displaySettings.backgroundColor.r, m_displaySettings.backgroundColor.g,
                 m_displaySettings.backgroundColor.b, m_displaySettings.backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    if(m_displaySettings.depthTest) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    // Backface culling
    if(m_displaySettings.backfaceCulling) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }

    // Build view and projection matrices
    QMatrix4x4 viewMatrix;
    viewMatrix.lookAt(QVector3D(m_camera.position[0], m_camera.position[1], m_camera.position[2]),
                      QVector3D(m_camera.target[0], m_camera.target[1], m_camera.target[2]),
                      QVector3D(m_camera.up[0], m_camera.up[1], m_camera.up[2]));

    QMatrix4x4 projMatrix;
    projMatrix.perspective(m_camera.fov, static_cast<float>(width) / static_cast<float>(height),
                           m_camera.nearPlane, m_camera.farPlane);

    QMatrix3x3 normalMatrix = viewMatrix.normalMatrix();

    // Render faces
    if(m_displaySettings.showFaces) {
        m_faceShader->bind();
        m_faceShader->setUniformValue(m_faceModelViewLoc, viewMatrix);
        m_faceShader->setUniformValue(m_faceProjectionLoc, projMatrix);
        m_faceShader->setUniformValue(m_faceNormalMatrixLoc, normalMatrix);
        m_faceShader->setUniformValue(
            m_faceLightDirLoc,
            QVector3D(m_mainLight.position[0], m_mainLight.position[1], m_mainLight.position[2]));
        m_faceShader->setUniformValue(
            m_faceLightColorLoc,
            QVector3D(m_mainLight.color[0], m_mainLight.color[1], m_mainLight.color[2]));
        m_faceShader->setUniformValue(
            m_faceSelectedColorLoc,
            QVector4D(m_displaySettings.selectedColor.r, m_displaySettings.selectedColor.g,
                      m_displaySettings.selectedColor.b, m_displaySettings.selectedColor.a));
        m_faceShader->setUniformValue(
            m_faceHighlightColorLoc,
            QVector4D(m_displaySettings.highlightColor.r, m_displaySettings.highlightColor.g,
                      m_displaySettings.highlightColor.b, m_displaySettings.highlightColor.a));

        for(const auto& buffers : m_meshBuffers) {
            if(buffers->visible && buffers->faceIndexCount > 0) {
                renderMeshFaces(*buffers);
            }
        }

        m_faceShader->release();
    }

    // Render edges with slight depth offset to prevent z-fighting
    if(m_displaySettings.showEdges) {
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        glLineWidth(m_displaySettings.edgeWidth);

        m_edgeShader->bind();
        m_edgeShader->setUniformValue(m_edgeModelViewLoc, viewMatrix);
        m_edgeShader->setUniformValue(m_edgeProjectionLoc, projMatrix);

        for(const auto& buffers : m_meshBuffers) {
            if(buffers->visible && buffers->edgeIndexCount > 0) {
                renderMeshEdges(*buffers);
            }
        }

        m_edgeShader->release();
        glDisable(GL_POLYGON_OFFSET_LINE);
    }
}

PickResult GLRenderer::pick(int x, int y, int width, int height) {
    // TODO: Implement GPU-based picking using color-coding or ray casting
    // For now, return no hit
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return PickResult::NoHit();
}

void GLRenderer::setHighlightedEntity(Geometry::EntityId entityId) {
    if(m_highlightedEntity == entityId) {
        return;
    }

    // Clear previous highlight
    if(auto* buf = findMeshBuffers(m_highlightedEntity)) {
        buf->highlighted = false;
    }

    m_highlightedEntity = entityId;

    // Set new highlight
    if(auto* buf = findMeshBuffers(entityId)) {
        buf->highlighted = true;
    }
}

void GLRenderer::setSelectedEntities(const std::vector<Geometry::EntityId>& entityIds) {
    // Clear previous selections
    for(const auto& buffers : m_meshBuffers) {
        buffers->selected = false;
    }

    m_selectedEntities = entityIds;

    // Set new selections
    for(Geometry::EntityId id : entityIds) {
        if(auto* buf = findMeshBuffers(id)) {
            buf->selected = true;
        }
    }
}

GLRenderer::MeshBuffers* GLRenderer::findMeshBuffers(Geometry::EntityId entityId) {
    for(auto& buf : m_meshBuffers) {
        if(buf->entityId == entityId) {
            return buf.get();
        }
    }
    return nullptr;
}

void GLRenderer::uploadMeshToGPU(const RenderMeshPtr& mesh, MeshBuffers& buffers) {
    buffers.visible = mesh->visible;
    buffers.selected = mesh->selected;
    buffers.highlighted = mesh->highlighted;
    buffers.baseColor = mesh->baseColor;

    // Upload face data
    if(!mesh->vertices.empty() && !mesh->faceIndices.empty()) {
        if(!buffers.faceVao.isCreated()) {
            buffers.faceVao.create();
        }
        buffers.faceVao.bind();

        if(!buffers.faceVbo.isCreated()) {
            buffers.faceVbo.create();
        }
        buffers.faceVbo.bind();
        buffers.faceVbo.allocate(mesh->vertices.data(),
                                 static_cast<int>(mesh->vertices.size() * sizeof(RenderVertex)));

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                              reinterpret_cast<void*>(offsetof(RenderVertex, position)));

        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                              reinterpret_cast<void*>(offsetof(RenderVertex, normal)));

        // Color attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex),
                              reinterpret_cast<void*>(offsetof(RenderVertex, color)));

        // Entity ID attribute
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(RenderVertex),
                              reinterpret_cast<void*>(offsetof(RenderVertex, entityId)));

        if(!buffers.faceIbo.isCreated()) {
            buffers.faceIbo.create();
        }
        buffers.faceIbo.bind();
        buffers.faceIbo.allocate(mesh->faceIndices.data(),
                                 static_cast<int>(mesh->faceIndices.size() * sizeof(uint32_t)));

        buffers.faceIndexCount = static_cast<int>(mesh->faceIndices.size());

        buffers.faceVao.release();
    }

    // Upload edge data
    if(!mesh->edgeVertices.empty() && !mesh->edgeIndices.empty()) {
        if(!buffers.edgeVao.isCreated()) {
            buffers.edgeVao.create();
        }
        buffers.edgeVao.bind();

        if(!buffers.edgeVbo.isCreated()) {
            buffers.edgeVbo.create();
        }
        buffers.edgeVbo.bind();
        buffers.edgeVbo.allocate(mesh->edgeVertices.data(),
                                 static_cast<int>(mesh->edgeVertices.size() * sizeof(EdgeVertex)));

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex),
                              reinterpret_cast<void*>(offsetof(EdgeVertex, position)));

        // Color attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex),
                              reinterpret_cast<void*>(offsetof(EdgeVertex, color)));

        // Entity ID attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(EdgeVertex),
                              reinterpret_cast<void*>(offsetof(EdgeVertex, entityId)));

        if(!buffers.edgeIbo.isCreated()) {
            buffers.edgeIbo.create();
        }
        buffers.edgeIbo.bind();
        buffers.edgeIbo.allocate(mesh->edgeIndices.data(),
                                 static_cast<int>(mesh->edgeIndices.size() * sizeof(uint32_t)));

        buffers.edgeIndexCount = static_cast<int>(mesh->edgeIndices.size());

        buffers.edgeVao.release();
    }
}

void GLRenderer::renderMeshFaces(MeshBuffers& buffers) {
    m_faceShader->setUniformValue(m_faceSelectedLoc, buffers.selected);
    m_faceShader->setUniformValue(m_faceHighlightedLoc, buffers.highlighted);

    buffers.faceVao.bind();
    glDrawElements(GL_TRIANGLES, buffers.faceIndexCount, GL_UNSIGNED_INT, nullptr);
    buffers.faceVao.release();
}

void GLRenderer::renderMeshEdges(MeshBuffers& buffers) {
    buffers.edgeVao.bind();
    glDrawElements(GL_LINES, buffers.edgeIndexCount, GL_UNSIGNED_INT, nullptr);
    buffers.edgeVao.release();
}

} // namespace OpenGeoLab::Render
