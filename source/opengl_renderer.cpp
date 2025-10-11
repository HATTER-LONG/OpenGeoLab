// opengl_renderer.cpp - Implementation of OpenGL renderer base classes
#include "opengl_renderer.h"
#include "logger.hpp"

#include <QMatrix4x4>

// ============================================================================
// OpenGLRendererBase Implementation
// ============================================================================

QOpenGLShaderProgram* OpenGLRendererBase::createShaderProgram(const char* vertexShader,
                                                              const char* fragmentShader) {
    auto* program = new QOpenGLShaderProgram();

    bool vertexOk = program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader);
    if(!vertexOk) {
        LOG_ERROR("Failed to compile vertex shader: {}", program->log().toStdString());
        delete program;
        return nullptr;
    }

    bool fragmentOk =
        program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader);
    if(!fragmentOk) {
        LOG_ERROR("Failed to compile fragment shader: {}", program->log().toStdString());
        delete program;
        return nullptr;
    }

    return program;
}

void OpenGLRendererBase::setupVertexAttribPointer(GLuint index,
                                                  GLint size,
                                                  GLsizei stride,
                                                  const void* offset) {
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride, offset);
}

// ============================================================================
// OpenGL3DRenderer Implementation
// ============================================================================

OpenGL3DRenderer::~OpenGL3DRenderer() { delete m_program; }

void OpenGL3DRenderer::setGeometryData(std::shared_ptr<GeometryData> geometryData) {
    m_geometryData = geometryData;
    if(m_initialized && geometryData) {
        createBuffers();
    }
}

void OpenGL3DRenderer::init() {
    LOG_TRACE("OpenGL3DRenderer::init() called, m_program={}", static_cast<void*>(m_program));

    if(!m_program && m_window) {
        LOG_INFO("Initializing OpenGL3DRenderer resources");

        QSGRendererInterface* rif = m_window->rendererInterface();
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::OpenGL);

        initializeOpenGLFunctions();

        // Create shader program
        m_program = createShaderProgram(getVertexShaderSource(), getFragmentShaderSource());
        if(!m_program) {
            LOG_ERROR("Failed to create shader program");
            return;
        }

        // Bind attribute locations
        m_program->bindAttributeLocation("aPos", 0);
        m_program->bindAttributeLocation("aNormal", 1);
        m_program->bindAttributeLocation("aColor", 2);

        bool linkOk = m_program->link();
        if(!linkOk) {
            LOG_ERROR("Failed to link shader program: {}", m_program->log().toStdString());
            delete m_program;
            m_program = nullptr;
            return;
        }

        LOG_INFO("Shader program linked successfully");

        // Create buffers if geometry data is available
        if(m_geometryData) {
            createBuffers();
        }

        m_initialized = true;
        LOG_INFO("OpenGL3DRenderer initialization complete");
    }
}

void OpenGL3DRenderer::createBuffers() {
    if(!m_geometryData) {
        LOG_WARN("No geometry data available for buffer creation");
        return;
    }

    LOG_DEBUG("Creating VBO and EBO for geometry with {} vertices, {} indices",
              m_geometryData->vertexCount(), m_geometryData->indexCount());

    // Create and populate vertex buffer
    m_vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(m_geometryData->vertices(),
                   m_geometryData->vertexCount() * 9 * sizeof(float)); // 9 floats per vertex
    m_vbo.release();

    LOG_DEBUG("VBO created with {} bytes", m_geometryData->vertexCount() * 9 * sizeof(float));

    // Create and populate index buffer
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

void OpenGL3DRenderer::setupVertexAttributes() {
    int stride = 9 * sizeof(float); // Each vertex: 3(pos) + 3(normal) + 3(color)

    // Position attribute (location = 0)
    setupVertexAttribPointer(0, 3, stride, nullptr);

    // Normal attribute (location = 1)
    setupVertexAttribPointer(1, 3, stride, reinterpret_cast<void*>(3 * sizeof(float)));

    // Color attribute (location = 2)
    setupVertexAttribPointer(2, 3, stride, reinterpret_cast<void*>(6 * sizeof(float)));
}

QMatrix4x4 OpenGL3DRenderer::calculateMVPMatrix() {
    QMatrix4x4 model, view, projection;

    // Model matrix - rotate the geometry
    model.rotate(m_rotation, 0.5f, 1.0f, 0.0f);

    // View matrix - camera position
    view.lookAt(QVector3D(0, 0, 3),  // Camera position
                QVector3D(0, 0, 0),  // Look at origin
                QVector3D(0, 1, 0)); // Up direction

    // Projection matrix - perspective projection
    float aspect =
        static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height());
    projection.perspective(45.0f, aspect, 0.1f, 100.0f);

    return projection * view * model;
}

const char* OpenGL3DRenderer::getVertexShaderSource() {
    return "attribute vec3 aPos;"
           "attribute vec3 aNormal;"
           "attribute vec3 aColor;"
           "uniform mat4 uMVP;"
           "uniform mat4 uModel;"
           "varying vec3 vColor;"
           "varying vec3 vNormal;"
           "varying vec3 vFragPos;"
           "void main() {"
           "    gl_Position = uMVP * vec4(aPos, 1.0);"
           "    vColor = aColor;"
           "    mat3 normalMatrix = mat3(uModel[0].xyz, uModel[1].xyz, uModel[2].xyz);"
           "    vNormal = normalMatrix * aNormal;"
           "    vFragPos = vec3(uModel * vec4(aPos, 1.0));"
           "}";
}

const char* OpenGL3DRenderer::getFragmentShaderSource() {
    return "varying vec3 vColor;"
           "varying vec3 vNormal;"
           "varying vec3 vFragPos;"
           "void main() {"
           "    vec3 lightPos = vec3(2.0, 2.0, 2.0);"
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
}

void OpenGL3DRenderer::paint() {
    if(!m_program || !m_geometryData) {
        LOG_WARN("Cannot paint: program={}, geometryData={}", static_cast<void*>(m_program),
                 static_cast<void*>(m_geometryData.get()));
        return;
    }

    LOG_TRACE("OpenGL3DRenderer::paint() called, rotation={}", m_rotation);

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

    // Calculate and set transformation matrices
    QMatrix4x4 model;
    model.rotate(m_rotation, 0.5f, 1.0f, 0.0f);
    QMatrix4x4 mvp = calculateMVPMatrix();

    m_program->setUniformValue("uMVP", mvp);
    m_program->setUniformValue("uModel", model);

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

    // Update rotation for animation
    m_rotation += 1.0;
    if(m_rotation >= 360.0) {
        m_rotation = 0.0;
    }

    // Trigger continuous updates
    if(m_window) {
        m_window->update();
    }
}
