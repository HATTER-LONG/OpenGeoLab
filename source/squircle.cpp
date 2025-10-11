// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "squircle.h"
#include "geometry.h"
#include "logger.hpp"

#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QtCore/QRunnable>
#include <QtQuick/qquickwindow.h>

//! [7]
Squircle::Squircle() : m_t(0), m_renderer(nullptr) {
    connect(this, &QQuickItem::windowChanged, this, &Squircle::handleWindowChanged);
}
//! [7]

//! [8]
void Squircle::setT(qreal t) {
    if(t == m_t) {
        return;
    }
    m_t = t;
    emit tChanged();
    if(window()) {
        window()->update();
    }
}
//! [8]

// 设置几何体类型
void Squircle::setGeometryType(const QString& type) {
    if(type == m_geometryType) {
        return;
    }
    LOG_INFO("Switching geometry type from '{}' to '{}'", m_geometryType.toStdString(),
             type.toStdString());
    m_geometryType = type;
    emit geometryTypeChanged();
    if(window()) {
        window()->update();
    }
}

//! [1]
void Squircle::handleWindowChanged(QQuickWindow* win) {
    if(win) {
        LOG_DEBUG("Window changed, setting up connections");
        connect(win, &QQuickWindow::beforeSynchronizing, this, &Squircle::sync,
                Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &Squircle::cleanup,
                Qt::DirectConnection);
        //! [1]
        //! [3]
        // Ensure we start with cleared to black. The squircle's blend mode relies on this.
        win->setColor(Qt::black);
    }
}
//! [3]

//! [6]
void Squircle::cleanup() {
    delete m_renderer;
    m_renderer = nullptr;
}

class CleanupJob : public QRunnable {
public:
    explicit CleanupJob(SquircleRenderer* renderer) : m_renderer(renderer) {}
    void run() override { delete m_renderer; }

private:
    SquircleRenderer* m_renderer;
};

void Squircle::releaseResources() {
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

SquircleRenderer::~SquircleRenderer() {
    delete m_program;
    delete m_cubeProgram;
}
//! [6]

//! [9]
void Squircle::sync() {
    if(!m_renderer) {
        LOG_INFO("Creating new SquircleRenderer");
        m_renderer = new SquircleRenderer();
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &SquircleRenderer::init,
                Qt::DirectConnection);
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer,
                &SquircleRenderer::paint, Qt::DirectConnection);
    }

    // 计算 Squircle 在窗口中的实际位置和大小
    QPointF scene_pos = mapToScene(QPointF(0, 0));
    QPoint offset(scene_pos.x() * window()->devicePixelRatio(),
                  scene_pos.y() * window()->devicePixelRatio());
    QSize size(width() * window()->devicePixelRatio(), height() * window()->devicePixelRatio());

    m_renderer->setViewportSize(size);
    m_renderer->setViewportOffset(offset);
    m_renderer->setT(m_t);
    m_renderer->setGeometryType(m_geometryType); // 传递几何体类型
    m_renderer->setWindow(window());
}
//! [9]

//! [4]
void SquircleRenderer::init() {
    LOG_TRACE("init() called, m_program={}, m_cubeProgram={}", static_cast<void*>(m_program),
              static_cast<void*>(m_cubeProgram));

    if(!m_program) {
        LOG_INFO("Initializing SquircleRenderer OpenGL resources");

        QSGRendererInterface* rif = m_window->rendererInterface();
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::OpenGL);

        initializeOpenGLFunctions();

        // 初始化几何数据
        m_squircleData = std::make_unique<SquircleData>();
        m_cubeData = std::make_unique<CubeData>();

        LOG_DEBUG("Geometry data initialized: Squircle vertices={}, Cube vertices={}, indices={}",
                  m_squircleData->vertexCount(), m_cubeData->vertexCount(),
                  m_cubeData->indexCount());

        // === Squircle 着色器和VBO ===
        const float values[] = {-1, -1, 1, -1, -1, 1, 1, 1};

        m_vbo.create();
        m_vbo.bind();
        m_vbo.allocate(values, sizeof(values));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

        m_program = new QOpenGLShaderProgram();
        m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                                    "attribute highp vec4 vertices;"
                                                    "varying highp vec2 coords;"
                                                    "void main() {"
                                                    "    gl_Position = vertices;"
                                                    "    coords = vertices.xy;"
                                                    "}");
        m_program->addCacheableShaderFromSourceCode(
            QOpenGLShader::Fragment,
            "uniform lowp float t;"
            "varying highp vec2 coords;"
            "void main() {"
            "    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));"
            "    i = smoothstep(t - 0.8, t + 0.8, i);"
            "    i = floor(i * 20.) / 20.;"
            "    gl_FragColor = vec4(coords * .5 + .5, i, i);"
            "}");

        m_program->bindAttributeLocation("vertices", 0);
        m_program->link();

        // === 立方体着色器和VBO/EBO ===
        // 创建顶点缓冲对象
        m_cubeVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_cubeVBO.create();
        m_cubeVBO.bind();
        m_cubeVBO.allocate(m_cubeData->vertices(),
                           m_cubeData->vertexCount() * 9 * sizeof(float)); // 每个顶点9个float
        m_cubeVBO.release();

        LOG_DEBUG("Cube VBO created with {} bytes", m_cubeData->vertexCount() * 9 * sizeof(float));

        // 创建索引缓冲对象
        m_cubeEBO = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_cubeEBO.create();
        m_cubeEBO.bind();
        m_cubeEBO.allocate(m_cubeData->indices(), m_cubeData->indexCount() * sizeof(unsigned int));
        m_cubeEBO.release();

        LOG_DEBUG("Cube EBO created with {} indices", m_cubeData->indexCount());

        // Cube shader with lighting effect
        m_cubeProgram = new QOpenGLShaderProgram();

        bool vertexOk = m_cubeProgram->addCacheableShaderFromSourceCode(
            QOpenGLShader::Vertex,
            "attribute vec3 aPos;"
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
            "}");

        if(!vertexOk) {
            LOG_ERROR("Failed to compile cube vertex shader: {}",
                      m_cubeProgram->log().toStdString());
        }

        bool fragmentOk = m_cubeProgram->addCacheableShaderFromSourceCode(
            QOpenGLShader::Fragment, "varying vec3 vColor;"
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
                                     "}");

        if(!fragmentOk) {
            LOG_ERROR("Failed to compile cube fragment shader: {}",
                      m_cubeProgram->log().toStdString());
        }

        m_cubeProgram->bindAttributeLocation("aPos", 0);
        m_cubeProgram->bindAttributeLocation("aNormal", 1);
        m_cubeProgram->bindAttributeLocation("aColor", 2);

        bool linkOk = m_cubeProgram->link();
        if(!linkOk) {
            LOG_ERROR("Failed to link cube shader program: {}", m_cubeProgram->log().toStdString());
        } else {
            LOG_INFO("Cube shader program linked successfully");
        }

        LOG_INFO("OpenGL initialization complete - Squircle and Cube shaders ready");
    }
}
//! [4]
//! [4] //! [5]
void SquircleRenderer::paint() {
    LOG_TRACE("paint() called, geometry type: {}", m_geometryType.toStdString());

    // 根据几何体类型选择渲染方法
    if(m_geometryType == "cube") {
        renderCube();
    } else {
        renderSquircle();
    }

    // 更新旋转角度(用于立方体动画)
    m_rotation += 1.0;
    if(m_rotation >= 360.0) {
        m_rotation = 0.0;
    }

    // 持续触发窗口更新以保持动画
    if(m_window) {
        m_window->update();
    }
}

// Squircle 渲染实现
void SquircleRenderer::renderSquircle() {
    LOG_TRACE("renderSquircle() called, t value: {}", m_t);

    m_window->beginExternalCommands();

    m_vbo.bind();
    m_program->bind();
    m_program->setUniformValue("t", static_cast<float>(m_t));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

    glViewport(m_viewportOffset.x(), m_viewportOffset.y(), m_viewportSize.width(),
               m_viewportSize.height());

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    m_program->release();

    m_window->endExternalCommands();
}

// 立方体渲染实现
void SquircleRenderer::renderCube() {
    LOG_TRACE("renderCube() called, rotation angle: {}", m_rotation);

    LOG_DEBUG("Checking cube resources: m_cubeProgram={}, m_cubeData={}, m_cubeVBO.isCreated={}, "
              "m_cubeEBO.isCreated={}",
              static_cast<void*>(m_cubeProgram), static_cast<void*>(m_cubeData.get()),
              m_cubeVBO.isCreated(), m_cubeEBO.isCreated());

    // 检查着色器程序是否存在且已链接
    if(!m_cubeProgram) {
        LOG_ERROR("Cube shader program is NULL!");
        return;
    }

    if(!m_cubeProgram->isLinked()) {
        LOG_ERROR("Cube shader program is not linked! Log: {}", m_cubeProgram->log().toStdString());
        return;
    }

    LOG_DEBUG("Cube shader program is valid and linked");

    m_window->beginExternalCommands();

    // 设置视口
    glViewport(m_viewportOffset.x(), m_viewportOffset.y(), m_viewportSize.width(),
               m_viewportSize.height());

    // 启用深度测试(立方体需要深度测试)
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // 清除颜色缓冲和深度缓冲 - 设置为深灰色背景以便看到立方体
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f); // 深灰蓝色背景
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    LOG_TRACE("Viewport set to: offset({}, {}), size({}, {})", m_viewportOffset.x(),
              m_viewportOffset.y(), m_viewportSize.width(), m_viewportSize.height());

    // 构建MVP矩阵
    QMatrix4x4 model, view, projection;

    // 模型矩阵 - 旋转立方体
    model.rotate(m_rotation, 0.5f, 1.0f, 0.0f); // 绕斜轴旋转

    // 视图矩阵 - 相机位置
    view.lookAt(QVector3D(0, 0, 3),  // 相机位置
                QVector3D(0, 0, 0),  // 看向原点
                QVector3D(0, 1, 0)); // 上方向

    // 投影矩阵 - 透视投影
    float aspect =
        static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height());
    projection.perspective(45.0f, aspect, 0.1f, 100.0f);

    QMatrix4x4 mvp = projection * view * model;

    // 先绑定着色器程序
    m_cubeProgram->bind();

    // 绑定立方体数据
    m_cubeVBO.bind();
    m_cubeEBO.bind();

    // 设置顶点属性
    int stride = 9 * sizeof(float); // 每个顶点: 3(pos) + 3(normal) + 3(color)

    glEnableVertexAttribArray(0); // 位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);

    glEnableVertexAttribArray(1); // 法向量
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(3 * sizeof(float)));

    glEnableVertexAttribArray(2); // 颜色
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(6 * sizeof(float)));

    // 设置矩阵uniform
    m_cubeProgram->setUniformValue("uMVP", mvp);
    m_cubeProgram->setUniformValue("uModel", model);

    // 绘制立方体
    glDrawElements(GL_TRIANGLES, m_cubeData->indexCount(), GL_UNSIGNED_INT, nullptr);

    LOG_TRACE("Drew cube with {} indices", m_cubeData->indexCount());

    // 清理
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    m_cubeProgram->release();

    m_window->endExternalCommands();
}
//! [5]
