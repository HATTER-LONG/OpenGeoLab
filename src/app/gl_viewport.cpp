/**
 * @file gl_viewport.cpp
 * @brief Implementation of GLViewport and GLViewportRenderer
 */

#include "gl_viewport.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtMath>

namespace OpenGeoLab::App {

// =============================================================================
// Shader Sources
// =============================================================================

static const char* VERTEX_SHADER_SOURCE = R"(
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

static const char* FRAGMENT_SHADER_SOURCE = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vColor;

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

    vec3 result = (ambient + diffuse + specular) * vColor.rgb;
    fragColor = vec4(result, vColor.a);
}
)";

static const char* GRID_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uMVPMatrix;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
}
)";

static const char* GRID_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vColor;
out vec4 fragColor;

void main() {
    fragColor = vColor;
}
)";

// =============================================================================
// GLViewport Implementation
// =============================================================================

GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);

    LOG_TRACE("GLViewport created");
}

GLViewport::~GLViewport() { LOG_TRACE("GLViewport destroyed"); }

QQuickFramebufferObject::Renderer* GLViewport::createRenderer() const {
    return new GLViewportRenderer(this);
}

Render::RenderService* GLViewport::renderService() const { return m_renderService; }

void GLViewport::setRenderService(Render::RenderService* service) {
    if(m_renderService == service) {
        return;
    }

    if(m_renderService) {
        disconnect(m_renderService, &Render::RenderService::sceneNeedsUpdate, this,
                   &GLViewport::onSceneNeedsUpdate);
        disconnect(m_renderService, &Render::RenderService::cameraChanged, this,
                   &GLViewport::onSceneNeedsUpdate);
    }

    m_renderService = service;

    if(m_renderService) {
        connect(m_renderService, &Render::RenderService::sceneNeedsUpdate, this,
                &GLViewport::onSceneNeedsUpdate);
        connect(m_renderService, &Render::RenderService::cameraChanged, this,
                &GLViewport::onSceneNeedsUpdate);
        m_cameraState = m_renderService->camera();
    }

    emit renderServiceChanged();
    update();
}

const Render::CameraState& GLViewport::cameraState() const { return m_cameraState; }

const Render::DocumentRenderData& GLViewport::renderData() const {
    static Render::DocumentRenderData empty;
    return m_renderService ? m_renderService->renderData() : empty;
}

void GLViewport::onSceneNeedsUpdate() {
    if(m_renderService) {
        m_cameraState = m_renderService->camera();
    }
    update();
}

void GLViewport::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->position();
    m_pressedButtons = event->buttons();
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    const QPointF delta = event->position() - m_lastMousePos;
    m_lastMousePos = event->position();

    if(m_pressedButtons & Qt::LeftButton) {
        // Left button: orbit
        orbitCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
    } else if(m_pressedButtons & Qt::MiddleButton) {
        // Middle button: pan
        panCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
    } else if(m_pressedButtons & Qt::RightButton) {
        // Right button: zoom
        zoomCamera(static_cast<float>(-delta.y()));
    }

    event->accept();
}

void GLViewport::mouseReleaseEvent(QMouseEvent* event) {
    m_pressedButtons = event->buttons();
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    const float delta = event->angleDelta().y() / 120.0f;
    zoomCamera(delta * 5.0f);
    event->accept();
}

void GLViewport::orbitCamera(float dx, float dy) {
    const float sensitivity = 0.5f;
    const float yaw = -dx * sensitivity;
    const float pitch = -dy * sensitivity;

    // Calculate the direction vector from camera to target
    QVector3D direction = m_cameraState.m_position - m_cameraState.m_target;
    const float distance = direction.length();

    // Convert to spherical coordinates
    float theta = qAtan2(direction.x(), direction.z());
    float phi = qAsin(qBound(-1.0f, direction.y() / distance, 1.0f));

    // Apply rotation
    theta += qDegreesToRadians(yaw);
    phi += qDegreesToRadians(pitch);

    // Clamp phi to avoid gimbal lock
    phi = qBound(-1.5f, phi, 1.5f);

    // Convert back to Cartesian coordinates
    direction.setX(distance * qCos(phi) * qSin(theta));
    direction.setY(distance * qSin(phi));
    direction.setZ(distance * qCos(phi) * qCos(theta));

    m_cameraState.m_position = m_cameraState.m_target + direction;

    if(m_renderService) {
        m_renderService->camera() = m_cameraState;
    }

    update();
}

void GLViewport::panCamera(float dx, float dy) {
    const float sensitivity = 0.01f;

    // Calculate right and up vectors
    const QVector3D forward = (m_cameraState.m_target - m_cameraState.m_position).normalized();
    const QVector3D right = QVector3D::crossProduct(forward, m_cameraState.m_up).normalized();
    const QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    // Calculate pan distance based on camera distance
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    const float panScale = distance * sensitivity;

    const QVector3D pan = right * (-dx * panScale) + up * (dy * panScale);
    m_cameraState.m_position += pan;
    m_cameraState.m_target += pan;

    if(m_renderService) {
        m_renderService->camera() = m_cameraState;
    }

    update();
}

void GLViewport::zoomCamera(float delta) {
    const float sensitivity = 0.1f;

    QVector3D direction = m_cameraState.m_position - m_cameraState.m_target;
    float distance = direction.length();

    // Apply zoom
    distance *= (1.0f - delta * sensitivity);
    distance = qMax(0.1f, distance);

    direction = direction.normalized() * distance;
    m_cameraState.m_position = m_cameraState.m_target + direction;

    if(m_renderService) {
        m_renderService->camera() = m_cameraState;
    }

    update();
}

// =============================================================================
// GLViewportRenderer Implementation
// =============================================================================

GLViewportRenderer::GLViewportRenderer(const GLViewport* viewport)
    : m_viewport(viewport), m_gridVBO(QOpenGLBuffer::VertexBuffer) {
    LOG_TRACE("GLViewportRenderer created");
}

GLViewportRenderer::~GLViewportRenderer() {
    LOG_TRACE("GLViewportRenderer destroyed");

    // Cleanup GPU resources
    if(m_gridVAO.isCreated()) {
        m_gridVAO.destroy();
    }
    if(m_gridVBO.isCreated()) {
        m_gridVBO.destroy();
    }

    for(auto& mesh : m_faceMeshBuffers) {
        if(mesh.vao && mesh.vao->isCreated())
            mesh.vao->destroy();
        if(mesh.vbo && mesh.vbo->isCreated())
            mesh.vbo->destroy();
        if(mesh.ebo && mesh.ebo->isCreated())
            mesh.ebo->destroy();
    }
    for(auto& mesh : m_edgeMeshBuffers) {
        if(mesh.vao && mesh.vao->isCreated())
            mesh.vao->destroy();
        if(mesh.vbo && mesh.vbo->isCreated())
            mesh.vbo->destroy();
        if(mesh.ebo && mesh.ebo->isCreated())
            mesh.ebo->destroy();
    }
    for(auto& mesh : m_vertexMeshBuffers) {
        if(mesh.vao && mesh.vao->isCreated())
            mesh.vao->destroy();
        if(mesh.vbo && mesh.vbo->isCreated())
            mesh.vbo->destroy();
        if(mesh.ebo && mesh.ebo->isCreated())
            mesh.ebo->destroy();
    }
}

QOpenGLFramebufferObject* GLViewportRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4); // Enable MSAA
    m_viewportSize = size;
    return new QOpenGLFramebufferObject(size, format);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = qobject_cast<GLViewport*>(item);
    if(!viewport) {
        return;
    }

    m_cameraState = viewport->cameraState();

    // Check if render data changed using version number
    const auto& newRenderData = viewport->renderData();
    if(newRenderData.m_version != m_renderData.m_version) {
        LOG_DEBUG("GLViewportRenderer: Render data changed, version {} -> {}, uploading {} meshes",
                  m_renderData.m_version, newRenderData.m_version, newRenderData.meshCount());
        m_renderData = newRenderData;
        m_needsDataUpload = true;
    }
}

void GLViewportRenderer::render() {
    if(!m_initialized) {
        initializeGL();
        m_initialized = true;
    }

    if(m_needsDataUpload) {
        uploadMeshData();
        m_needsDataUpload = false;
    }

    // Clear background
    glClearColor(0.15f, 0.15f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calculate matrices
    const float aspectRatio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    const QMatrix4x4 projection = m_cameraState.projectionMatrix(aspectRatio);
    const QMatrix4x4 view = m_cameraState.viewMatrix();
    const QMatrix4x4 model; // Identity
    const QMatrix4x4 mvp = projection * view * model;
    const QMatrix3x3 normalMatrix = model.normalMatrix();

    // Render grid
    renderGrid();

    // Render meshes
    if(m_shaderProgram && m_shaderProgram->isLinked()) {
        m_shaderProgram->bind();
        m_shaderProgram->setUniformValue(m_mvpMatrixLoc, mvp);
        m_shaderProgram->setUniformValue(m_modelMatrixLoc, model);
        m_shaderProgram->setUniformValue(m_normalMatrixLoc, normalMatrix);
        m_shaderProgram->setUniformValue(m_lightPosLoc, m_cameraState.m_position);
        m_shaderProgram->setUniformValue(m_viewPosLoc, m_cameraState.m_position);

        renderMeshes();

        m_shaderProgram->release();
    }

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void GLViewportRenderer::initializeGL() {
    initializeOpenGLFunctions();
    setupShaders();

    // Setup grid
    m_gridVAO.create();
    m_gridVBO.create();

    // Generate grid vertices
    std::vector<float> gridVertices;
    const float gridSize = 100.0f;
    const float gridStep = 5.0f;
    const int gridLines = static_cast<int>(gridSize / gridStep);

    // Grid color (subtle gray)
    const float gridColor[4] = {0.3f, 0.3f, 0.35f, 0.5f};
    const float axisColorX[4] = {0.8f, 0.2f, 0.2f, 0.8f}; // Red for X
    const float axisColorZ[4] = {0.2f, 0.2f, 0.8f, 0.8f}; // Blue for Z

    for(int i = -gridLines; i <= gridLines; ++i) {
        const float pos = i * gridStep;

        // Choose color based on whether this is an axis line
        const float* colorX = (i == 0) ? axisColorZ : gridColor;
        const float* colorZ = (i == 0) ? axisColorX : gridColor;

        // Lines parallel to X axis
        gridVertices.insert(gridVertices.end(), {-gridSize, 0.0f, pos});
        gridVertices.insert(gridVertices.end(), {colorX[0], colorX[1], colorX[2], colorX[3]});
        gridVertices.insert(gridVertices.end(), {gridSize, 0.0f, pos});
        gridVertices.insert(gridVertices.end(), {colorX[0], colorX[1], colorX[2], colorX[3]});

        // Lines parallel to Z axis
        gridVertices.insert(gridVertices.end(), {pos, 0.0f, -gridSize});
        gridVertices.insert(gridVertices.end(), {colorZ[0], colorZ[1], colorZ[2], colorZ[3]});
        gridVertices.insert(gridVertices.end(), {pos, 0.0f, gridSize});
        gridVertices.insert(gridVertices.end(), {colorZ[0], colorZ[1], colorZ[2], colorZ[3]});
    }

    m_gridVertexCount = static_cast<int>(gridVertices.size()) / 7;

    m_gridVAO.bind();
    m_gridVBO.bind();
    m_gridVBO.allocate(gridVertices.data(), static_cast<int>(gridVertices.size() * sizeof(float)));

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);

    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));

    m_gridVBO.release();
    m_gridVAO.release();

    LOG_DEBUG("GLViewportRenderer: OpenGL initialized");
}

void GLViewportRenderer::setupShaders() {
    // Main shader program
    m_shaderProgram = std::make_unique<QOpenGLShaderProgram>();
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER_SOURCE);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAGMENT_SHADER_SOURCE);
    if(!m_shaderProgram->link()) {
        LOG_ERROR("Failed to link main shader: {}", m_shaderProgram->log().toStdString());
    }

    m_mvpMatrixLoc = m_shaderProgram->uniformLocation("uMVPMatrix");
    m_modelMatrixLoc = m_shaderProgram->uniformLocation("uModelMatrix");
    m_normalMatrixLoc = m_shaderProgram->uniformLocation("uNormalMatrix");
    m_lightPosLoc = m_shaderProgram->uniformLocation("uLightPos");
    m_viewPosLoc = m_shaderProgram->uniformLocation("uViewPos");
    m_pointSizeLoc = m_shaderProgram->uniformLocation("uPointSize");

    // Grid shader program
    m_gridShader = std::make_unique<QOpenGLShaderProgram>();
    m_gridShader->addShaderFromSourceCode(QOpenGLShader::Vertex, GRID_VERTEX_SHADER);
    m_gridShader->addShaderFromSourceCode(QOpenGLShader::Fragment, GRID_FRAGMENT_SHADER);
    if(!m_gridShader->link()) {
        LOG_ERROR("Failed to link grid shader: {}", m_gridShader->log().toStdString());
    }
}

void GLViewportRenderer::uploadMeshData() {
    // Clear old buffers
    for(auto& mesh : m_faceMeshBuffers) {
        if(mesh.vao && mesh.vao->isCreated())
            mesh.vao->destroy();
        if(mesh.vbo && mesh.vbo->isCreated())
            mesh.vbo->destroy();
        if(mesh.ebo && mesh.ebo->isCreated())
            mesh.ebo->destroy();
    }
    m_faceMeshBuffers.clear();

    for(auto& mesh : m_edgeMeshBuffers) {
        if(mesh.vao && mesh.vao->isCreated())
            mesh.vao->destroy();
        if(mesh.vbo && mesh.vbo->isCreated())
            mesh.vbo->destroy();
        if(mesh.ebo && mesh.ebo->isCreated())
            mesh.ebo->destroy();
    }
    m_edgeMeshBuffers.clear();

    for(auto& mesh : m_vertexMeshBuffers) {
        if(mesh.vao && mesh.vao->isCreated())
            mesh.vao->destroy();
        if(mesh.vbo && mesh.vbo->isCreated())
            mesh.vbo->destroy();
        if(mesh.ebo && mesh.ebo->isCreated())
            mesh.ebo->destroy();
    }
    m_vertexMeshBuffers.clear();

    // Upload face meshes
    for(const auto& mesh : m_renderData.m_faceMeshes) {
        if(!mesh.isValid())
            continue;

        MeshBuffers buffers;
        buffers.vao->create();
        buffers.vbo->create();
        if(mesh.isIndexed()) {
            buffers.ebo->create();
        }

        uploadMesh(mesh, *buffers.vao, *buffers.vbo, *buffers.ebo);

        buffers.vertexCount = static_cast<int>(mesh.vertexCount());
        buffers.indexCount = static_cast<int>(mesh.indexCount());
        buffers.primitiveType = mesh.m_primitiveType;

        m_faceMeshBuffers.push_back(std::move(buffers));
    }

    // Upload edge meshes
    for(const auto& mesh : m_renderData.m_edgeMeshes) {
        if(!mesh.isValid())
            continue;

        MeshBuffers buffers;
        buffers.vao->create();
        buffers.vbo->create();
        if(mesh.isIndexed()) {
            buffers.ebo->create();
        }

        uploadMesh(mesh, *buffers.vao, *buffers.vbo, *buffers.ebo);

        buffers.vertexCount = static_cast<int>(mesh.vertexCount());
        buffers.indexCount = static_cast<int>(mesh.indexCount());
        buffers.primitiveType = mesh.m_primitiveType;

        m_edgeMeshBuffers.push_back(std::move(buffers));
    }

    // Upload vertex meshes
    for(const auto& mesh : m_renderData.m_vertexMeshes) {
        if(!mesh.isValid())
            continue;

        MeshBuffers buffers;
        buffers.vao->create();
        buffers.vbo->create();

        uploadMesh(mesh, *buffers.vao, *buffers.vbo, *buffers.ebo);

        buffers.vertexCount = static_cast<int>(mesh.vertexCount());
        buffers.primitiveType = mesh.m_primitiveType;

        m_vertexMeshBuffers.push_back(std::move(buffers));
    }

    LOG_DEBUG("Uploaded {} face meshes, {} edge meshes, {} vertex meshes", m_faceMeshBuffers.size(),
              m_edgeMeshBuffers.size(), m_vertexMeshBuffers.size());
}

void GLViewportRenderer::uploadMesh(const Render::RenderMesh& mesh,
                                    QOpenGLVertexArrayObject& vao,
                                    QOpenGLBuffer& vbo,
                                    QOpenGLBuffer& ebo) {
    vao.bind();
    vbo.bind();

    // Upload vertex data
    vbo.allocate(mesh.m_vertices.data(),
                 static_cast<int>(mesh.m_vertices.size() * sizeof(Render::RenderVertex)));

    // Setup vertex attributes
    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Render::RenderVertex),
                          reinterpret_cast<void*>(offsetof(Render::RenderVertex, m_position)));

    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Render::RenderVertex),
                          reinterpret_cast<void*>(offsetof(Render::RenderVertex, m_normal)));

    // Color (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Render::RenderVertex),
                          reinterpret_cast<void*>(offsetof(Render::RenderVertex, m_color)));

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

void GLViewportRenderer::renderMeshes() {
    auto getPrimitiveType = [](Render::RenderPrimitiveType type) -> GLenum {
        switch(type) {
        case Render::RenderPrimitiveType::Points:
            return GL_POINTS;
        case Render::RenderPrimitiveType::Lines:
            return GL_LINES;
        case Render::RenderPrimitiveType::LineStrip:
            return GL_LINE_STRIP;
        case Render::RenderPrimitiveType::Triangles:
            return GL_TRIANGLES;
        case Render::RenderPrimitiveType::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case Render::RenderPrimitiveType::TriangleFan:
            return GL_TRIANGLE_FAN;
        default:
            return GL_TRIANGLES;
        }
    };

    // Enable shader-controlled point size
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Set default point size for faces/edges
    m_shaderProgram->setUniformValue(m_pointSizeLoc, 1.0f);

    // Render face meshes
    for(auto& buffers : m_faceMeshBuffers) {
        buffers.vao->bind();
        if(buffers.indexCount > 0) {
            buffers.ebo->bind();
            glDrawElements(getPrimitiveType(buffers.primitiveType), buffers.indexCount,
                           GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(getPrimitiveType(buffers.primitiveType), 0, buffers.vertexCount);
        }
        buffers.vao->release();
    }

    // Render edge meshes (with line width)
    glLineWidth(2.0f);
    for(auto& buffers : m_edgeMeshBuffers) {
        buffers.vao->bind();
        if(buffers.indexCount > 0) {
            buffers.ebo->bind();
            glDrawElements(getPrimitiveType(buffers.primitiveType), buffers.indexCount,
                           GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(getPrimitiveType(buffers.primitiveType), 0, buffers.vertexCount);
        }
        buffers.vao->release();
    }

    // Render vertex meshes (with larger point size)
    m_shaderProgram->setUniformValue(m_pointSizeLoc, 5.0f);
    for(auto& buffers : m_vertexMeshBuffers) {
        buffers.vao->bind();
        glDrawArrays(GL_POINTS, 0, buffers.vertexCount);
        buffers.vao->release();
    }
}

void GLViewportRenderer::renderGrid() {
    if(!m_gridShader || !m_gridShader->isLinked()) {
        return;
    }

    const float aspectRatio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    const QMatrix4x4 mvp = m_cameraState.projectionMatrix(aspectRatio) * m_cameraState.viewMatrix();

    m_gridShader->bind();
    m_gridShader->setUniformValue("uMVPMatrix", mvp);

    glLineWidth(1.0f);
    m_gridVAO.bind();
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVAO.release();

    m_gridShader->release();
}

} // namespace OpenGeoLab::App
