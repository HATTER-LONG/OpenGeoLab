// Copyright (C) 2025 OpenGeoLab
// SPDX-License-Identifier: MIT

#include "triangle.h"
#include "logger.hpp"
#include <QOpenGLContext>
#include <QtCore/QRunnable>
#include <QtMath>
#include <unordered_map>

// ==================== TriangleRenderer Implementation ====================

TriangleRenderer::TriangleRenderer() : m_color("red"), m_angle(0.0), m_colorVec(1.0f, 0.0f, 0.0f) {}

TriangleRenderer::~TriangleRenderer() { delete m_program; }

void TriangleRenderer::setColor(const QString& color) {
    if(m_color == color)
        return;
    m_color = color;
    updateColorUniform();
}

void TriangleRenderer::setAngle(qreal angle) { m_angle = angle; }

void TriangleRenderer::setViewportSize(const QSize& size) { m_viewportSize = size; }

void TriangleRenderer::setWindow(QQuickWindow* window) { m_window = window; }

void TriangleRenderer::updateColorUniform() {
    static const std::unordered_map<QString, QVector3D> colorMap = {
        {"red", QVector3D(1.0f, 0.0f, 0.0f)},     {"green", QVector3D(0.0f, 1.0f, 0.0f)},
        {"blue", QVector3D(0.0f, 0.0f, 1.0f)},    {"yellow", QVector3D(1.0f, 1.0f, 0.0f)},
        {"magenta", QVector3D(1.0f, 0.0f, 1.0f)}, {"cyan", QVector3D(0.0f, 1.0f, 1.0f)},
        {"white", QVector3D(1.0f, 1.0f, 1.0f)}};

    auto it = colorMap.find(m_color);
    if(it != colorMap.end()) {
        m_colorVec = it->second;
    }
}

void TriangleRenderer::init() {
    if(!m_program) {
        QSGRendererInterface* rif = m_window->rendererInterface();
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::OpenGL);

        initializeOpenGLFunctions();

        // 三角形顶点数据 (x, y)
        const float vertices[] = {0.0f,  0.5f,   // 顶部
                                  -0.5f, -0.5f,  // 左下
                                  0.5f,  -0.5f}; // 右下

        m_vbo.create();
        m_vbo.bind();
        m_vbo.allocate(vertices, sizeof(vertices));

        m_program = new QOpenGLShaderProgram();

        // 顶点着色器 - 支持旋转变换
        bool vshaderOk = m_program->addCacheableShaderFromSourceCode(
            QOpenGLShader::Vertex, "attribute highp vec4 vertices;\n"
                                   "uniform highp float angle;\n"
                                   "void main() {\n"
                                   "    float rad = radians(angle);\n"
                                   "    float c = cos(rad);\n"
                                   "    float s = sin(rad);\n"
                                   "    mat2 rotation = mat2(c, -s, s, c);\n"
                                   "    vec2 pos = rotation * vertices.xy;\n"
                                   "    gl_Position = vec4(pos, 0.0, 1.0);\n"
                                   "}");

        // 片段着色器 - 使用统一颜色
        bool fshaderOk = m_program->addCacheableShaderFromSourceCode(
            QOpenGLShader::Fragment, "uniform lowp vec3 color;\n"
                                     "void main() {\n"
                                     "    gl_FragColor = vec4(color, 1.0);\n"
                                     "}");

        m_program->bindAttributeLocation("vertices", 0);
        bool linkOk = m_program->link();

        if(!vshaderOk || !fshaderOk || !linkOk) {
            LOG_ERROR("Triangle shader compilation/linking failed: {}",
                      m_program->log().toStdString());
        } else {
            LOG_INFO("Triangle shader compiled and linked successfully");
        }

        updateColorUniform();
    }
}

void TriangleRenderer::paint() {
    if(!m_program || !m_window)
        return;

    // Play nice with the RHI
    m_window->beginExternalCommands();

    m_vbo.bind();
    m_program->bind();

    // 设置uniform变量
    m_program->setUniformValue("color", m_colorVec);
    m_program->setUniformValue("angle", static_cast<float>(m_angle));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 绘制三角形
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);
    m_program->release();
    m_vbo.release();

    m_window->endExternalCommands();

    // 第一次绘制时打印日志
    static bool first_paint = true;
    if(first_paint) {
        LOG_INFO("Triangle first paint - viewport: {}x{}, color: ({}, {}, {}), angle: {}",
                 m_viewportSize.width(), m_viewportSize.height(), m_colorVec.x(), m_colorVec.y(),
                 m_colorVec.z(), m_angle);
        first_paint = false;
    }
}

// ==================== TriangleItem Implementation ====================

TriangleItem::TriangleItem() {
    // 设置标志,允许此 Item 进行自定义渲染
    setFlag(ItemHasContents, true);
    connect(this, &QQuickItem::windowChanged, this, &TriangleItem::handleWindowChanged);
}

void TriangleItem::setColor(const QString& color) {
    if(m_color == color)
        return;
    m_color = color;
    emit colorChanged();
    if(window())
        window()->update();
}

void TriangleItem::setAngle(qreal angle) {
    if(qFuzzyCompare(m_angle, angle))
        return;
    m_angle = angle;
    emit angleChanged();
    if(window())
        window()->update();
}

void TriangleItem::handleWindowChanged(QQuickWindow* win) {
    if(win) {
        // 确保窗口背景色不会遮挡我们的渲染
        connect(win, &QQuickWindow::beforeSynchronizing, this, &TriangleItem::sync,
                Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &TriangleItem::cleanup,
                Qt::DirectConnection);
    }
}

void TriangleItem::cleanup() {
    delete m_renderer;
    m_renderer = nullptr;
}

class CleanupJob : public QRunnable {
public:
    CleanupJob(TriangleRenderer* renderer) : m_renderer(renderer) {}
    void run() override { delete m_renderer; }

private:
    TriangleRenderer* m_renderer;
};

void TriangleItem::releaseResources() {
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

void TriangleItem::sync() {
    if(!m_renderer) {
        m_renderer = new TriangleRenderer();
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &TriangleRenderer::init,
                Qt::DirectConnection);
        // 改为 afterRenderPassRecording,在场景图渲染之后绘制
        connect(window(), &QQuickWindow::afterRenderPassRecording, m_renderer,
                &TriangleRenderer::paint, Qt::DirectConnection);
    }

    // 使用整个窗口大小
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_renderer->setColor(m_color);
    m_renderer->setAngle(m_angle);
    m_renderer->setWindow(window());
}
