// cube_renderer.cpp - Implementation of cube renderer
#include "cube_renderer.h"
#include "logger.hpp"

#include <QtCore/QRunnable>
#include <QtQuick/qquickwindow.h>

// ============================================================================
// CubeRenderer Implementation
// ============================================================================

CubeRenderer::CubeRenderer() {
    LOG_DEBUG("CubeRenderer constructor called");
    initializeCubeGeometry();
}

void CubeRenderer::initializeCubeGeometry() {
    // Create cube geometry data
    auto cubeData = std::make_shared<CubeData>();
    setGeometryData(cubeData);

    LOG_INFO("Cube geometry initialized with {} vertices and {} indices", cubeData->vertexCount(),
             cubeData->indexCount());
}

void CubeRenderer::init() {
    LOG_DEBUG("CubeRenderer::init() called");

    // Call base class initialization
    OpenGL3DRenderer::init();

    if(m_initialized) {
        LOG_INFO("CubeRenderer initialized successfully");
    }
}

// ============================================================================
// Cube3D QML Item Implementation
// ============================================================================

Cube3D::Cube3D() {
    LOG_DEBUG("Cube3D constructor called");
    connect(this, &QQuickItem::windowChanged, this, &Cube3D::handleWindowChanged);
}

void Cube3D::handleWindowChanged(QQuickWindow* win) {
    if(win) {
        LOG_DEBUG("Cube3D window changed, setting up connections");

        // Connect to Qt Quick scene graph signals
        connect(win, &QQuickWindow::beforeSynchronizing, this, &Cube3D::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &Cube3D::cleanup,
                Qt::DirectConnection);

        // Set background color
        win->setColor(Qt::black);

        LOG_INFO("Cube3D window connections established");
    }
}

void Cube3D::sync() {
    if(!m_renderer) {
        LOG_INFO("Creating new CubeRenderer");

        m_renderer = new CubeRenderer();

        // Connect renderer to scene graph signals
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &CubeRenderer::init,
                Qt::DirectConnection);
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer,
                &CubeRenderer::paint, Qt::DirectConnection);

        emit rendererReady();
    }

    // Calculate item position and size in window coordinates
    QPointF scenePos = mapToScene(QPointF(0, 0));
    QPoint offset(scenePos.x() * window()->devicePixelRatio(),
                  scenePos.y() * window()->devicePixelRatio());
    QSize size(width() * window()->devicePixelRatio(), height() * window()->devicePixelRatio());

    // Update renderer state
    m_renderer->setViewportSize(size);
    m_renderer->setViewportOffset(offset);
    m_renderer->setWindow(window());

    LOG_TRACE("Cube3D sync: offset=({},{}), size=({}x{})", offset.x(), offset.y(), size.width(),
              size.height());
}

void Cube3D::cleanup() {
    LOG_INFO("Cube3D cleanup called");
    delete m_renderer;
    m_renderer = nullptr;
}

/**
 * @brief Helper class for cleaning up renderer in render thread
 */
class CleanupJob : public QRunnable {
public:
    explicit CleanupJob(CubeRenderer* renderer) : m_renderer(renderer) {
        LOG_DEBUG("CleanupJob created for renderer");
    }

    void run() override {
        LOG_DEBUG("CleanupJob running, deleting renderer");
        delete m_renderer;
    }

private:
    CubeRenderer* m_renderer;
};

void Cube3D::releaseResources() {
    LOG_INFO("Cube3D releasing resources");
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}
