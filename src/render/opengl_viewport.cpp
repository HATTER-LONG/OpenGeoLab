/**
 * @file opengl_viewport.cpp
 * @brief Implementation of OpenGL viewport for QML
 */

#include "render/opengl_viewport.hpp"

#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>

#include <cmath>

namespace OpenGeoLab::Render {

// =============================================================================
// Shader Sources
// =============================================================================

namespace {
const char* VERTEX_SHADER_SOURCE = R"(
    #version 330 core
    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec3 aNormal;
    layout(location = 2) in vec4 aColor;

    uniform mat4 uModelMatrix;
    uniform mat4 uViewMatrix;
    uniform mat4 uProjectionMatrix;
    uniform mat3 uNormalMatrix;

    out vec3 vNormal;
    out vec3 vFragPos;
    out vec4 vColor;

    void main() {
        vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
        vFragPos = worldPos.xyz;
        vNormal = uNormalMatrix * aNormal;
        vColor = aColor;
        gl_Position = uProjectionMatrix * uViewMatrix * worldPos;
    }
)";

const char* FRAGMENT_SHADER_SOURCE = R"(
    #version 330 core
    in vec3 vNormal;
    in vec3 vFragPos;
    in vec4 vColor;

    uniform vec3 uLightPos;
    uniform vec3 uViewPos;
    uniform vec3 uLightColor;
    uniform float uAmbientStrength;
    uniform float uSpecularStrength;

    out vec4 fragColor;

    void main() {
        // Ambient
        vec3 ambient = uAmbientStrength * uLightColor;

        // Diffuse
        vec3 norm = normalize(vNormal);
        vec3 lightDir = normalize(uLightPos - vFragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * uLightColor;

        // Specular
        vec3 viewDir = normalize(uViewPos - vFragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = uSpecularStrength * spec * uLightColor;

        vec3 result = (ambient + diffuse + specular) * vColor.rgb;
        fragColor = vec4(result, vColor.a);
    }
)";

} // anonymous namespace

// =============================================================================
// OpenGLViewport
// =============================================================================

OpenGLViewport::OpenGLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsInputMethod, true);
    setMirrorVertically(true);
}

OpenGLViewport::~OpenGLViewport() = default;

QQuickFramebufferObject::Renderer* OpenGLViewport::createRenderer() const {
    return new OpenGLViewportRenderer();
}

void OpenGLViewport::setCameraPosition(const QVector3D& position) {
    if(m_cameraPosition != position) {
        m_cameraPosition = position;
        emit cameraPositionChanged();
        update();
    }
}

void OpenGLViewport::setCameraTarget(const QVector3D& target) {
    if(m_cameraTarget != target) {
        m_cameraTarget = target;
        emit cameraTargetChanged();
        update();
    }
}

void OpenGLViewport::setFieldOfView(float fov) {
    if(!qFuzzyCompare(m_fieldOfView, fov)) {
        m_fieldOfView = fov;
        emit fieldOfViewChanged();
        update();
    }
}

void OpenGLViewport::setRenderScene(const RenderScene& scene) {
    m_renderScene = scene;
    m_hasModel = !scene.isEmpty();
    m_sceneNeedsUpdate = true;
    emit hasModelChanged();
    update();
}

void OpenGLViewport::clearModel() {
    m_renderScene.clear();
    m_hasModel = false;
    m_sceneNeedsUpdate = true;
    emit hasModelChanged();
    update();
}

void OpenGLViewport::resetCamera() {
    setCameraPosition(QVector3D(0.0f, 0.0f, 5.0f));
    setCameraTarget(QVector3D(0.0f, 0.0f, 0.0f));
    setFieldOfView(45.0f);
}

void OpenGLViewport::fitToView() {
    if(!m_hasModel || m_renderScene.isEmpty()) {
        resetCamera();
        return;
    }

    // Calculate bounding box center and size
    const auto& bbox = m_renderScene.m_boundingBox;
    if(!bbox.isValid()) {
        resetCamera();
        return;
    }

    auto center = bbox.center();
    float diagonal = static_cast<float>(bbox.diagonal());

    // Position camera to see the entire model
    float distance = diagonal * 1.5f;
    QVector3D newTarget(static_cast<float>(center.m_x), static_cast<float>(center.m_y),
                        static_cast<float>(center.m_z));
    QVector3D newPosition = newTarget + QVector3D(0.0f, 0.0f, distance);

    setCameraTarget(newTarget);
    setCameraPosition(newPosition);
}

void OpenGLViewport::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->position();
    if(event->button() == Qt::LeftButton) {
        m_rotating = true;
    } else if(event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_panning = true;
    }
    event->accept();
}

void OpenGLViewport::mouseMoveEvent(QMouseEvent* event) {
    QPointF delta = event->position() - m_lastMousePos;
    m_lastMousePos = event->position();

    if(m_rotating) {
        // Orbit camera around target
        float angleX = static_cast<float>(delta.x()) * 0.5f;
        float angleY = static_cast<float>(delta.y()) * 0.5f;

        QVector3D direction = m_cameraPosition - m_cameraTarget;
        float radius = direction.length();

        // Convert to spherical coordinates
        float theta = std::atan2(direction.x(), direction.z());
        float phi = std::acos(direction.y() / radius);

        theta -= angleX * 0.01f;
        phi += angleY * 0.01f;

        // Clamp phi to avoid flipping
        phi = std::clamp(phi, 0.1f, 3.04f);

        // Convert back to Cartesian
        direction.setX(radius * std::sin(phi) * std::sin(theta));
        direction.setY(radius * std::cos(phi));
        direction.setZ(radius * std::sin(phi) * std::cos(theta));

        setCameraPosition(m_cameraTarget + direction);
    } else if(m_panning) {
        // Pan camera
        QVector3D forward = (m_cameraTarget - m_cameraPosition).normalized();
        QVector3D right = QVector3D::crossProduct(forward, QVector3D(0, 1, 0)).normalized();
        QVector3D up = QVector3D::crossProduct(right, forward).normalized();

        float panSpeed = 0.01f * (m_cameraPosition - m_cameraTarget).length();
        QVector3D panOffset = right * static_cast<float>(-delta.x()) * panSpeed +
                              up * static_cast<float>(delta.y()) * panSpeed;

        setCameraPosition(m_cameraPosition + panOffset);
        setCameraTarget(m_cameraTarget + panOffset);
    }

    event->accept();
}

void OpenGLViewport::mouseReleaseEvent(QMouseEvent* event) {
    m_rotating = false;
    m_panning = false;
    event->accept();
}

void OpenGLViewport::wheelEvent(QWheelEvent* event) {
    // Zoom camera
    float delta = event->angleDelta().y() / 120.0f;
    float zoomFactor = 1.0f - delta * 0.1f;

    QVector3D direction = m_cameraPosition - m_cameraTarget;
    direction *= zoomFactor;

    // Prevent zooming too close or too far
    float distance = direction.length();
    if(distance > 0.1f && distance < 1000.0f) {
        setCameraPosition(m_cameraTarget + direction);
    }

    event->accept();
}

// =============================================================================
// OpenGLViewportRenderer
// =============================================================================

OpenGLViewportRenderer::OpenGLViewportRenderer() {
    // Initialization deferred to first render call
}

OpenGLViewportRenderer::~OpenGLViewportRenderer() {
    m_defaultVAO.destroy();
    m_defaultVBO.destroy();
    m_sceneVAO.destroy();
    m_sceneVBO.destroy();
    m_sceneIBO.destroy();
}

QOpenGLFramebufferObject* OpenGLViewportRenderer::createFramebufferObject(const QSize& size) {
    m_viewportSize = size;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

void OpenGLViewportRenderer::render() {
    if(!m_initialized) {
        initializeGL();
        m_initialized = true;
    }

    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Update projection matrix
    float aspect =
        static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height());
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(m_fieldOfView, aspect, 0.01f, 1000.0f);

    // Update view matrix
    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(m_cameraPosition, m_cameraTarget, QVector3D(0, 1, 0));

    // Model matrix (identity for now)
    m_modelMatrix.setToIdentity();

    if(m_hasModel && !m_renderScene.isEmpty()) {
        renderScene();
    } else {
        renderDefaultTriangle();
    }
}

void OpenGLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = static_cast<OpenGLViewport*>(item);

    m_cameraPosition = viewport->m_cameraPosition;
    m_cameraTarget = viewport->m_cameraTarget;
    m_fieldOfView = viewport->m_fieldOfView;
    m_hasModel = viewport->m_hasModel;

    if(viewport->m_sceneNeedsUpdate) {
        m_renderScene = viewport->m_renderScene;
        m_buffersNeedUpdate = true;
        viewport->m_sceneNeedsUpdate = false;
    }
}

void OpenGLViewportRenderer::initializeGL() {
    initializeOpenGLFunctions();
    initializeShaders();
    initializeDefaultTriangle();

    LOG_INFO("OpenGL Viewport initialized");
    LOG_INFO("  OpenGL Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOG_INFO("  GLSL Version: {}",
             reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
}

void OpenGLViewportRenderer::initializeShaders() {
    m_shaderProgram = std::make_unique<QOpenGLShaderProgram>();

    if(!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER_SOURCE)) {
        LOG_ERROR("Failed to compile vertex shader: {}", m_shaderProgram->log().toStdString());
        return;
    }

    if(!m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAGMENT_SHADER_SOURCE)) {
        LOG_ERROR("Failed to compile fragment shader: {}", m_shaderProgram->log().toStdString());
        return;
    }

    if(!m_shaderProgram->link()) {
        LOG_ERROR("Failed to link shader program: {}", m_shaderProgram->log().toStdString());
        return;
    }

    LOG_DEBUG("Shaders compiled and linked successfully");
}

void OpenGLViewportRenderer::initializeDefaultTriangle() {
    // Default triangle vertices with position, normal, and color
    // clang-format off
    static const float triangleVertices[] = {
        // Position          Normal           Color
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.2f, 0.6f, 0.9f, 1.0f,  // Bottom-left (blue)
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.2f, 0.8f, 0.4f, 1.0f,  // Bottom-right (green)
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.9f, 0.3f, 0.3f, 1.0f   // Top (red)
    };
    // clang-format on

    m_defaultVAO.create();
    m_defaultVAO.bind();

    m_defaultVBO.create();
    m_defaultVBO.bind();
    m_defaultVBO.allocate(triangleVertices, sizeof(triangleVertices));

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), nullptr);

    // Normal attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));

    // Color attribute (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                          reinterpret_cast<void*>(6 * sizeof(float)));

    m_defaultVAO.release();
    m_defaultVBO.release();

    LOG_DEBUG("Default triangle initialized");
}

void OpenGLViewportRenderer::updateBuffers() {
    if(m_renderScene.isEmpty()) {
        return;
    }

    // Collect all vertices and indices
    std::vector<float> vertexData;
    std::vector<uint32_t> indexData;
    uint32_t indexOffset = 0;

    for(const auto& mesh : m_renderScene.m_meshes) {
        for(const auto& vertex : mesh.m_vertices) {
            // Position
            vertexData.push_back(vertex.m_position[0]);
            vertexData.push_back(vertex.m_position[1]);
            vertexData.push_back(vertex.m_position[2]);
            // Normal
            vertexData.push_back(vertex.m_normal[0]);
            vertexData.push_back(vertex.m_normal[1]);
            vertexData.push_back(vertex.m_normal[2]);
            // Color
            vertexData.push_back(vertex.m_color[0]);
            vertexData.push_back(vertex.m_color[1]);
            vertexData.push_back(vertex.m_color[2]);
            vertexData.push_back(vertex.m_color[3]);
        }

        for(const auto& index : mesh.m_indices) {
            indexData.push_back(index + indexOffset);
        }

        indexOffset += static_cast<uint32_t>(mesh.m_vertices.size());
    }

    if(vertexData.empty()) {
        return;
    }

    // Create/update VAO and buffers
    if(!m_sceneVAO.isCreated()) {
        m_sceneVAO.create();
    }
    m_sceneVAO.bind();

    if(!m_sceneVBO.isCreated()) {
        m_sceneVBO.create();
    }
    m_sceneVBO.bind();
    m_sceneVBO.allocate(vertexData.data(), static_cast<int>(vertexData.size() * sizeof(float)));

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), nullptr);

    // Normal attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));

    // Color attribute (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                          reinterpret_cast<void*>(6 * sizeof(float)));

    if(!indexData.empty()) {
        if(!m_sceneIBO.isCreated()) {
            m_sceneIBO.create();
        }
        m_sceneIBO.bind();
        m_sceneIBO.allocate(indexData.data(),
                            static_cast<int>(indexData.size() * sizeof(uint32_t)));
    }

    m_sceneVAO.release();

    LOG_DEBUG("Scene buffers updated: {} vertices, {} indices", vertexData.size() / 10,
              indexData.size());
}

void OpenGLViewportRenderer::renderDefaultTriangle() {
    if(!m_shaderProgram || !m_shaderProgram->isLinked()) {
        return;
    }

    m_shaderProgram->bind();

    // Set uniforms
    m_shaderProgram->setUniformValue("uModelMatrix", m_modelMatrix);
    m_shaderProgram->setUniformValue("uViewMatrix", m_viewMatrix);
    m_shaderProgram->setUniformValue("uProjectionMatrix", m_projectionMatrix);

    QMatrix3x3 normalMatrix = m_modelMatrix.normalMatrix();
    m_shaderProgram->setUniformValue("uNormalMatrix", normalMatrix);

    // Lighting
    m_shaderProgram->setUniformValue("uLightPos", QVector3D(5.0f, 5.0f, 5.0f));
    m_shaderProgram->setUniformValue("uViewPos", m_cameraPosition);
    m_shaderProgram->setUniformValue("uLightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shaderProgram->setUniformValue("uAmbientStrength", 0.3f);
    m_shaderProgram->setUniformValue("uSpecularStrength", 0.5f);

    m_defaultVAO.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_defaultVAO.release();

    m_shaderProgram->release();
}

void OpenGLViewportRenderer::renderScene() {
    if(!m_shaderProgram || !m_shaderProgram->isLinked()) {
        return;
    }

    if(m_buffersNeedUpdate) {
        updateBuffers();
        m_buffersNeedUpdate = false;
    }

    if(!m_sceneVAO.isCreated()) {
        return;
    }

    m_shaderProgram->bind();

    // Set uniforms
    m_shaderProgram->setUniformValue("uModelMatrix", m_modelMatrix);
    m_shaderProgram->setUniformValue("uViewMatrix", m_viewMatrix);
    m_shaderProgram->setUniformValue("uProjectionMatrix", m_projectionMatrix);

    QMatrix3x3 normalMatrix = m_modelMatrix.normalMatrix();
    m_shaderProgram->setUniformValue("uNormalMatrix", normalMatrix);

    // Lighting
    m_shaderProgram->setUniformValue("uLightPos", m_cameraPosition + QVector3D(5.0f, 5.0f, 5.0f));
    m_shaderProgram->setUniformValue("uViewPos", m_cameraPosition);
    m_shaderProgram->setUniformValue("uLightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shaderProgram->setUniformValue("uAmbientStrength", 0.3f);
    m_shaderProgram->setUniformValue("uSpecularStrength", 0.5f);

    m_sceneVAO.bind();

    // Calculate total index count
    size_t totalIndices = 0;
    for(const auto& mesh : m_renderScene.m_meshes) {
        totalIndices += mesh.m_indices.size();
    }

    if(totalIndices > 0) {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(totalIndices), GL_UNSIGNED_INT, nullptr);
    } else {
        // Fallback to non-indexed rendering
        size_t totalVertices = 0;
        for(const auto& mesh : m_renderScene.m_meshes) {
            totalVertices += mesh.m_vertices.size();
        }
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(totalVertices));
    }

    m_sceneVAO.release();
    m_shaderProgram->release();
}

} // namespace OpenGeoLab::Render
