// opengl_renderer.cpp - Implementation of OpenGL renderer
#include "opengl_renderer.h"
#include "logger.hpp"

#include <QMatrix4x4>

// ============================================================================
// OpenGLRenderer Implementation
// ============================================================================

OpenGLRenderer::~OpenGLRenderer() { delete m_program; }

void OpenGLRenderer::setGeometryData(std::shared_ptr<GeometryData> geometry_data) {
    m_geometryData = geometry_data;
    m_needsBufferUpdate = true;

    LOG_INFO("Geometry data set: {} vertices, {} indices",
             geometry_data ? geometry_data->vertexCount() : 0,
             geometry_data ? geometry_data->indexCount() : 0);
}

void OpenGLRenderer::setColor(const QColor& color) {
    if(m_color != color) {
        m_color = color;
        LOG_DEBUG("Color override set to: ({}, {}, {}, {})", color.red(), color.green(),
                  color.blue(), color.alpha());
    }
}

void OpenGLRenderer::setRotation(qreal rotation_x, qreal rotation_y) {
    m_rotationX = rotation_x;
    m_rotationY = rotation_y;
    LOG_TRACE("Rotation set to: X={}, Y={}", rotation_x, rotation_y);
}

void OpenGLRenderer::setZoom(qreal zoom) {
    // Clamp zoom to extended range (0.01x to 100x)
    m_zoom = qBound(0.01, zoom, 100.0);
    LOG_TRACE("Zoom set to: {}", m_zoom);
}

void OpenGLRenderer::setPan(qreal pan_x, qreal pan_y) {
    m_panX = pan_x;
    m_panY = pan_y;
    LOG_TRACE("Pan set to: X={}, Y={}", m_panX, m_panY);
}

void OpenGLRenderer::init() {
    LOG_TRACE("OpenGLRenderer::init() called");

    if(!m_program && m_window) {
        LOG_INFO("Initializing OpenGL renderer resources");

        QSGRendererInterface* rif = m_window->rendererInterface();
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::OpenGL);

        initializeOpenGLFunctions();

        // Create shader program
        createShaderProgram();
        if(!m_program) {
            LOG_ERROR("Failed to create shader program");
            return;
        }

        // Create buffers if geometry data is available
        if(m_geometryData && m_needsBufferUpdate) {
            createBuffers();
            m_needsBufferUpdate = false;
        }

        m_initialized = true;
        LOG_INFO("OpenGL renderer initialization complete");
    }
}

void OpenGLRenderer::createShaderProgram() {
    m_program = new QOpenGLShaderProgram();

    // Vertex shader with color override support
    const char* vertex_shader =
        "attribute vec3 aPos;"
        "attribute vec3 aNormal;"
        "attribute vec3 aColor;"
        "uniform mat4 uMVP;"
        "uniform mat4 uModel;"
        "uniform vec4 uColorOverride;" // Alpha = 0 means use vertex colors
        "varying vec3 vColor;"
        "varying vec3 vNormal;"
        "varying vec3 vFragPos;"
        "void main() {"
        "    gl_Position = uMVP * vec4(aPos, 1.0);"
        "    if (uColorOverride.a > 0.0) {"
        "        vColor = uColorOverride.rgb;"
        "    } else {"
        "        vColor = aColor;"
        "    }"
        "    mat3 normalMatrix = mat3(uModel[0].xyz, uModel[1].xyz, uModel[2].xyz);"
        "    vNormal = normalMatrix * aNormal;"
        "    vFragPos = vec3(uModel * vec4(aPos, 1.0));"
        "}";

    // Fragment shader with simple lighting
    const char* fragment_shader = "varying vec3 vColor;"
                                  "varying vec3 vNormal;"
                                  "varying vec3 vFragPos;"
                                  "void main() {"
                                  "    vec3 lightPos = vec3(50.0, 50.0, 50.0);"
                                  "    vec3 lightColor = vec3(1.0, 1.0, 1.0);"
                                  "    float ambientStrength = 0.3;"
                                  "    vec3 ambient = ambientStrength * lightColor;"
                                  "    vec3 norm = normalize(vNormal);"
                                  "    vec3 lightDir = normalize(lightPos - vFragPos);"
                                  "    float diff = max(dot(norm, lightDir), 0.0);"
                                  "    vec3 diffuse = diff * lightColor;"
                                  "    vec3 result = (ambient + diffuse) * vColor;"
                                  "    gl_FragColor = vec4(result, 1.0);"
                                  "}";

    bool vertex_ok =
        m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader);
    if(!vertex_ok) {
        LOG_ERROR("Failed to compile vertex shader: {}", m_program->log().toStdString());
        delete m_program;
        m_program = nullptr;
        return;
    }

    bool fragment_ok =
        m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader);
    if(!fragment_ok) {
        LOG_ERROR("Failed to compile fragment shader: {}", m_program->log().toStdString());
        delete m_program;
        m_program = nullptr;
        return;
    }

    // Bind attribute locations
    m_program->bindAttributeLocation("aPos", 0);
    m_program->bindAttributeLocation("aNormal", 1);
    m_program->bindAttributeLocation("aColor", 2);

    bool link_ok = m_program->link();
    if(!link_ok) {
        LOG_ERROR("Failed to link shader program: {}", m_program->log().toStdString());
        delete m_program;
        m_program = nullptr;
        return;
    }

    LOG_INFO("Shader program created and linked successfully");
}

void OpenGLRenderer::createBuffers() {
    if(!m_geometryData) {
        LOG_WARN("No geometry data available for buffer creation");
        return;
    }

    LOG_DEBUG("Creating VBO and EBO for geometry with {} vertices, {} indices",
              m_geometryData->vertexCount(), m_geometryData->indexCount());

    // Destroy old buffers if they exist
    if(m_vbo.isCreated()) {
        m_vbo.destroy();
    }
    if(m_ebo.isCreated()) {
        m_ebo.destroy();
    }

    // Create and populate vertex buffer
    m_vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(m_geometryData->vertices(),
                   m_geometryData->vertexCount() * 9 * sizeof(float)); // 9 floats per vertex
    m_vbo.release();

    LOG_DEBUG("VBO created with {} bytes", m_geometryData->vertexCount() * 9 * sizeof(float));

    // Create and populate index buffer if available
    if(m_geometryData->indices() && m_geometryData->indexCount() > 0) {
        m_ebo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_ebo.create();
        m_ebo.bind();
        m_ebo.allocate(m_geometryData->indices(),
                       m_geometryData->indexCount() * sizeof(unsigned int));
        m_ebo.release();

        LOG_DEBUG("EBO created with {} indices", m_geometryData->indexCount());
    }
}

void OpenGLRenderer::setupVertexAttributes() {
    int stride = 9 * sizeof(float); // Each vertex: 3(pos) + 3(normal) + 3(color)

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);

    // Normal attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(3 * sizeof(float)));

    // Color attribute (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(6 * sizeof(float)));
}

QMatrix4x4 OpenGLRenderer::calculateMVPMatrix() const {
    QMatrix4x4 model, view, projection;

    // Model matrix - apply rotations around model center
    model.setToIdentity();
    model.rotate(m_rotationY, 0.0f, 1.0f, 0.0f); // Rotate around Y axis (left-right drag)
    model.rotate(m_rotationX, 1.0f, 0.0f, 0.0f); // Rotate around X axis (up-down drag)

    // View matrix - camera position with zoom and pan
    // Camera orbit: fixed distance * zoom factor
    float camera_distance = 3.0f / m_zoom; // Zoom in = closer camera

    // Apply pan by translating the look-at target
    QVector3D look_at_target(m_panX, m_panY, 0.0f);
    QVector3D camera_position(m_panX, m_panY, camera_distance);

    view.lookAt(camera_position,     // Camera position (affected by zoom and pan)
                look_at_target,      // Look at target (affected by pan)
                QVector3D(0, 1, 0)); // Up direction

    // Projection matrix - perspective projection
    // Extended far clipping plane to support wide zoom range
    float aspect =
        static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height());
    projection.perspective(45.0f, aspect, 0.01f, 10000.0f); // Near: 0.01, Far: 10000

    return projection * view * model;
}

void OpenGLRenderer::paint() {
    if(!m_program || !m_geometryData) {
        LOG_TRACE("Skipping paint: program={}, geometryData={}", static_cast<void*>(m_program),
                  static_cast<void*>(m_geometryData.get()));
        return;
    }

    LOG_TRACE("OpenGLRenderer::paint() called");

    // Check if buffers need to be updated (e.g., geometry changed)
    if(m_needsBufferUpdate) {
        createBuffers();
        m_needsBufferUpdate = false;
    }

    m_window->beginExternalCommands();

    // Setup viewport
    glViewport(m_viewportOffset.x(), m_viewportOffset.y(), m_viewportSize.width(),
               m_viewportSize.height());

    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Clear with dark background
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind shader program
    m_program->bind();

    // Bind buffers
    m_vbo.bind();
    if(m_geometryData->indices()) {
        m_ebo.bind();
    }

    // Setup vertex attributes
    setupVertexAttributes();

    // Set transformation matrices
    QMatrix4x4 model;
    model.setToIdentity();
    model.rotate(m_rotationY, 0.0f, 1.0f, 0.0f); // Rotate around Y axis (left-right drag)
    model.rotate(m_rotationX, 1.0f, 0.0f, 0.0f); // Rotate around X axis (up-down drag)

    QMatrix4x4 mvp = calculateMVPMatrix();

    m_program->setUniformValue("uMVP", mvp);
    m_program->setUniformValue("uModel", model);

    // Set color override
    QVector4D color_override(m_color.redF(), m_color.greenF(), m_color.blueF(), m_color.alphaF());
    m_program->setUniformValue("uColorOverride", color_override);

    // Draw geometry
    if(m_geometryData->indices() && m_geometryData->indexCount() > 0) {
        glDrawElements(GL_TRIANGLES, m_geometryData->indexCount(), GL_UNSIGNED_INT, nullptr);
        LOG_TRACE("Drew geometry with {} indices", m_geometryData->indexCount());
    } else {
        glDrawArrays(GL_TRIANGLES, 0, m_geometryData->vertexCount());
        LOG_TRACE("Drew geometry with {} vertices", m_geometryData->vertexCount());
    }

    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    m_program->release();

    m_window->endExternalCommands();
}
