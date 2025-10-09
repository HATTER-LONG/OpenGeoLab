/**
 * @file triangle.cpp
 * @brief Implementation of OpenGL triangle rendering for Qt Quick
 */

#include "triangle.h"
#include "logger.hpp"
#include <QDateTime>
#include <QElapsedTimer>
#include <QOpenGLContext>
#include <QtCore/QRunnable>
#include <QtMath>
#include <unordered_map>

// ==================== TriangleRenderer Implementation ====================

TriangleRenderer::TriangleRenderer() : m_color("red"), m_angle(0.0), m_colorVec(1.0f, 0.0f, 0.0f) {
    LOG_DEBUG("TriangleRenderer created");
}

TriangleRenderer::~TriangleRenderer() {
    delete m_program;
    LOG_DEBUG("TriangleRenderer destroyed");
}

void TriangleRenderer::setColor(const QString& color) {
    if(m_color == color) {
        return;
    }
    m_color = color;
    updateColorUniform();
}

void TriangleRenderer::setAngle(qreal angle) { m_angle = angle; }

void TriangleRenderer::setViewportSize(const QSize& size) { m_viewportSize = size; }

void TriangleRenderer::setViewportPosition(const QPoint& pos) { m_viewportPos = pos; }

void TriangleRenderer::setWindow(QQuickWindow* window) { m_window = window; }

void TriangleRenderer::updateColorUniform() {
    // Predefined color mappings from color names to RGB vectors
    static const std::unordered_map<QString, QVector3D> color_map = {
        {"red", QVector3D(1.0f, 0.0f, 0.0f)},     {"green", QVector3D(0.0f, 1.0f, 0.0f)},
        {"blue", QVector3D(0.0f, 0.0f, 1.0f)},    {"yellow", QVector3D(1.0f, 1.0f, 0.0f)},
        {"magenta", QVector3D(1.0f, 0.0f, 1.0f)}, {"cyan", QVector3D(0.0f, 1.0f, 1.0f)},
        {"white", QVector3D(1.0f, 1.0f, 1.0f)}};

    auto it = color_map.find(m_color);
    if(it != color_map.end()) {
        m_colorVec = it->second;
        LOG_DEBUG("Color updated to: {} -> RGB({}, {}, {})", m_color.toStdString(), m_colorVec.x(),
                  m_colorVec.y(), m_colorVec.z());
    } else {
        LOG_WARN("Unknown color name: {}, using red as default", m_color.toStdString());
        m_colorVec = QVector3D(1.0f, 0.0f, 0.0f);
    }
}

void TriangleRenderer::init() {
    if(!m_program) {
        // Verify we're using OpenGL rendering backend
        QSGRendererInterface* rif = m_window->rendererInterface();
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::OpenGL);

        initializeOpenGLFunctions();

        // Triangle vertex data (x, y) in normalized device coordinates
        // Forms an equilateral triangle centered at origin
        const float vertices[] = {
            0.0f,  0.5f,  // Top vertex
            -0.5f, -0.5f, // Bottom-left vertex
            0.5f,  -0.5f  // Bottom-right vertex
        };

        // Create and populate vertex buffer object
        m_vbo.create();
        m_vbo.bind();
        m_vbo.allocate(vertices, sizeof(vertices));

        m_program = new QOpenGLShaderProgram();

        // Vertex shader - applies rotation transformation
        // Uses a 2D rotation matrix to rotate vertices around the origin
        const char* vertex_shader_source = "attribute highp vec4 vertices;\n"
                                           "uniform highp float angle;\n"
                                           "void main() {\n"
                                           "    float rad = radians(angle);\n"
                                           "    float c = cos(rad);\n"
                                           "    float s = sin(rad);\n"
                                           "    mat2 rotation = mat2(c, -s, s, c);\n"
                                           "    vec2 pos = rotation * vertices.xy;\n"
                                           "    gl_Position = vec4(pos, 0.0, 1.0);\n"
                                           "}";

        bool vshader_ok = m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                                                      vertex_shader_source);

        // Fragment shader - applies uniform color to all fragments
        const char* fragment_shader_source = "uniform lowp vec3 color;\n"
                                             "void main() {\n"
                                             "    gl_FragColor = vec4(color, 1.0);\n"
                                             "}";

        bool fshader_ok = m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,
                                                                      fragment_shader_source);

        // Bind vertex attribute location before linking
        m_program->bindAttributeLocation("vertices", 0);
        bool link_ok = m_program->link();

        if(!vshader_ok || !fshader_ok || !link_ok) {
            LOG_ERROR("Triangle shader compilation/linking failed: {}",
                      m_program->log().toStdString());
        } else {
            LOG_INFO("Triangle shader compiled and linked successfully");
        }

        updateColorUniform();
    }
}

void TriangleRenderer::paint() {
    if(!m_program || !m_window) {
        return;
    }

    // Notify Qt's RHI (Rendering Hardware Interface) that we're issuing custom OpenGL commands
    // This ensures proper synchronization with Qt Quick's scene graph
    m_window->beginExternalCommands();

    // Set viewport to the specified rendering area (accounting for device pixel ratio)
    glViewport(m_viewportPos.x(), m_viewportPos.y(), m_viewportSize.width(),
               m_viewportSize.height());

    // Enable scissor test to restrict rendering to the specified region
    // This prevents rendering outside the designated area
    glEnable(GL_SCISSOR_TEST);
    glScissor(m_viewportPos.x(), m_viewportPos.y(), m_viewportSize.width(),
              m_viewportSize.height());

    // Clear the current rendering region to prevent visual artifacts (ghosting)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Bind resources and set up rendering state
    m_vbo.bind();
    m_program->bind();

    // Set shader uniform variables
    m_program->setUniformValue("color", m_colorVec);
    m_program->setUniformValue("angle", static_cast<float>(m_angle));

    // Configure vertex attribute pointer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

    // Set up rendering state for 2D blending
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the triangle (3 vertices)
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Clean up rendering state
    glDisableVertexAttribArray(0);
    glDisable(GL_SCISSOR_TEST);
    m_program->release();
    m_vbo.release();

    // Signal end of external OpenGL commands
    m_window->endExternalCommands();

    // Log first paint information for debugging
    static bool first_paint = true;
    if(first_paint) {
        LOG_INFO("Triangle first paint - viewport pos: ({}, {}), size: {}x{}, "
                 "color: ({:.2f}, {:.2f}, {:.2f}), angle: {:.1f}°",
                 m_viewportPos.x(), m_viewportPos.y(), m_viewportSize.width(),
                 m_viewportSize.height(), m_colorVec.x(), m_colorVec.y(), m_colorVec.z(), m_angle);
        first_paint = false;
    }
}

// ==================== TriangleItem Implementation ====================

TriangleItem::TriangleItem() {
    // Enable custom rendering for this Item
    // This flag tells Qt Quick that this item needs custom OpenGL rendering
    setFlag(ItemHasContents, true);

    // Set up window change handler to properly initialize/cleanup renderer
    connect(this, &QQuickItem::windowChanged, this, &TriangleItem::handleWindowChanged);

    LOG_DEBUG("TriangleItem created");
}

void TriangleItem::setColor(const QString& color) {
    if(m_color == color) {
        return;
    }
    m_color = color;
    emit colorChanged();
    if(window()) {
        window()->update();
    }
}

void TriangleItem::setAngle(qreal angle) {
    if(qFuzzyCompare(m_angle, angle)) {
        return;
    }
    m_angle = angle;
    emit angleChanged();
    if(window()) {
        window()->update();
    }
}

void TriangleItem::handleWindowChanged(QQuickWindow* win) {
    if(win) {
        // Connect to Qt Quick's scene graph signals for proper synchronization

        // beforeSynchronizing: Called before scene graph synchronization
        // This is where we sync QML property changes to the renderer
        connect(win, &QQuickWindow::beforeSynchronizing, this, &TriangleItem::sync,
                Qt::DirectConnection);

        // sceneGraphInvalidated: Called when the scene graph is being destroyed
        // This is where we clean up OpenGL resources
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &TriangleItem::cleanup,
                Qt::DirectConnection);

        LOG_DEBUG("TriangleItem connected to window signals");
    }
}

void TriangleItem::cleanup() {
    delete m_renderer;
    m_renderer = nullptr;
    LOG_DEBUG("TriangleItem renderer cleaned up");
}

/**
 * @class CleanupJob
 * @brief QRunnable for cleaning up renderer in the render thread
 *
 * This helper class ensures that the renderer is deleted in the correct thread
 * (the render thread) to avoid OpenGL context issues.
 */
class CleanupJob : public QRunnable {
public:
    explicit CleanupJob(TriangleRenderer* renderer) : m_renderer(renderer) { setAutoDelete(true); }

    void run() override {
        delete m_renderer;
        LOG_DEBUG("CleanupJob: Renderer deleted in render thread");
    }

private:
    TriangleRenderer* m_renderer;
};

void TriangleItem::releaseResources() {
    // Schedule cleanup in the render thread to ensure proper OpenGL context
    if(m_renderer && window()) {
        window()->scheduleRenderJob(new CleanupJob(m_renderer),
                                    QQuickWindow::BeforeSynchronizingStage);
        m_renderer = nullptr;
    }
}

void TriangleItem::sync() {
    // Create renderer if it doesn't exist yet
    if(!m_renderer) {
        m_renderer = new TriangleRenderer();

        // Connect renderer to window's rendering signals
        // beforeRendering: Initialize OpenGL resources before first render
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &TriangleRenderer::init,
                Qt::DirectConnection);

        // afterRenderPassRecording: Render our custom content after Qt Quick's scene graph
        // This ensures our triangle appears on top of the QML scene
        connect(window(), &QQuickWindow::afterRenderPassRecording, m_renderer,
                &TriangleRenderer::paint, Qt::DirectConnection);

        LOG_INFO("TriangleItem renderer created and connected");
    }

    // Calculate Item's position and size in window coordinates (accounting for device pixel ratio)
    qreal dpr = window()->devicePixelRatio();
    QPointF item_pos = mapToScene(QPointF(0, 0));

    // OpenGL coordinate system origin is at bottom-left, need to flip Y coordinate
    // Qt Quick coordinate system origin is at top-left
    int window_height = window()->height() * dpr;
    int gl_x = static_cast<int>(item_pos.x() * dpr);
    int gl_y = static_cast<int>(window_height - (item_pos.y() + height()) * dpr);
    int gl_width = static_cast<int>(width() * dpr);
    int gl_height = static_cast<int>(height() * dpr);

    // Update renderer with current state
    m_renderer->setViewportPosition(QPoint(gl_x, gl_y));
    m_renderer->setViewportSize(QSize(gl_width, gl_height));
    m_renderer->setColor(m_color);
    m_renderer->setAngle(m_angle);
    m_renderer->setWindow(window());

    // Update FPS counter (sync is called before each frame)
    updateFps();
}

void TriangleItem::updateFps() {
    m_frameCount++;

    // Get current time in milliseconds since epoch
    qint64 current_time = QDateTime::currentMSecsSinceEpoch();

    // Initialize timing on first call
    if(m_lastFpsTime == 0) {
        m_lastFpsTime = current_time;
        return;
    }

    // Update FPS every second
    qint64 elapsed = current_time - m_lastFpsTime;
    if(elapsed >= 1000) {
        // Calculate FPS: frames / elapsed seconds
        m_fps = qRound(m_frameCount * 1000.0 / elapsed);

        // Reset counters for next measurement period
        m_frameCount = 0;
        m_lastFpsTime = current_time;

        // Notify QML of FPS update
        emit fpsChanged();

        // Log FPS periodically (every 10 seconds) for monitoring
        static int log_count = 0;
        if(log_count++ % 10 == 0) {
            LOG_INFO("OpenGL Render FPS: {}", m_fps);
        }
    }
}
