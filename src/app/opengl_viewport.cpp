/**
 * @file opengl_viewport.cpp
 * @brief Implementation of GLViewport and GLViewportRenderer
 */

#include "app/opengl_viewport.hpp"
#include "util/logger.hpp"

#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtCore/QMetaObject>
#include <QtMath>

namespace OpenGeoLab::App {

// =============================================================================
// GLViewport Implementation
// =============================================================================

GLViewport::GLViewport(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
    setMirrorVertically(true);

    auto& scene_controller = Render::RenderSceneController::instance();
    m_cameraState = scene_controller.camera();
    m_hasGeometry = scene_controller.hasGeometry();

    // Bridge service events (Util::Signal) onto the Qt/GUI thread
    m_sceneNeedsUpdateConn = scene_controller.subscribeSceneNeedsUpdate([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
    m_cameraChangedConn = scene_controller.subscribeCameraChanged([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onSceneNeedsUpdate, Qt::QueuedConnection);
    });
    m_geometryChangedConn = scene_controller.subscribeGeometryChanged([this]() {
        QMetaObject::invokeMethod(this, &GLViewport::onServiceGeometryChanged,
                                  Qt::QueuedConnection);
    });

    LOG_TRACE("GLViewport created");
}

GLViewport::~GLViewport() { LOG_TRACE("GLViewport destroyed"); }

QQuickFramebufferObject::Renderer* GLViewport::createRenderer() const {
    return new GLViewportRenderer(this);
}

bool GLViewport::hasGeometry() const { return m_hasGeometry; }

const Render::CameraState& GLViewport::cameraState() const { return m_cameraState; }

const Render::DocumentRenderData& GLViewport::renderData() const {
    static Render::DocumentRenderData empty;
    return Render::RenderSceneController::instance().renderData();
}

void GLViewport::onSceneNeedsUpdate() {
    m_cameraState = Render::RenderSceneController::instance().camera();
    update();
}

void GLViewport::onServiceGeometryChanged() {
    const bool new_has_geometry = Render::RenderSceneController::instance().hasGeometry();
    if(new_has_geometry != m_hasGeometry) {
        m_hasGeometry = new_has_geometry;
        emit hasGeometryChanged();
    }

    emit geometryChanged();
    update();
}

void GLViewport::keyPressEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}
void GLViewport::mousePressEvent(QMouseEvent* event) {
    if(!hasFocus()) {
        forceActiveFocus();
    }
    m_lastMousePos = event->position();
    m_pressedButtons = event->buttons();
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent* event) {
    const QPointF delta = event->position() - m_lastMousePos;
    m_lastMousePos = event->position();

    if((m_pressedButtons & Qt::LeftButton) && (m_pressedModifiers & Qt::ControlModifier)) {
        // Ctrl + Left button: orbit
        orbitCamera(static_cast<float>(delta.x()), static_cast<float>(delta.y()));

    } else if((m_pressedButtons & Qt::LeftButton) && (m_pressedModifiers & Qt::ShiftModifier) ||
              m_pressedButtons & Qt::MiddleButton) {
        // Shift + Left button: pan
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
void GLViewport::keyReleaseEvent(QKeyEvent* event) {
    m_pressedModifiers = event->modifiers();
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent* event) {
    if((m_pressedModifiers & Qt::ControlModifier)) {
        const float delta = event->angleDelta().y() / 120.0f;
        zoomCamera(delta * 5.0f);
    }
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

    Render::RenderSceneController::instance().setCamera(m_cameraState, false);
    update();
}

void GLViewport::panCamera(float dx, float dy) {
    const float sensitivity = 0.005f;

    // Calculate right and up vectors
    const QVector3D forward = (m_cameraState.m_target - m_cameraState.m_position).normalized();
    const QVector3D right = QVector3D::crossProduct(forward, m_cameraState.m_up).normalized();
    const QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    // Calculate pan distance based on camera distance
    const float distance = (m_cameraState.m_position - m_cameraState.m_target).length();
    const float pan_scale = distance * sensitivity;

    const QVector3D pan = right * (-dx * pan_scale) + up * (dy * pan_scale);
    m_cameraState.m_position += pan;
    m_cameraState.m_target += pan;

    Render::RenderSceneController::instance().setCamera(m_cameraState, false);

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

    m_cameraState.updateClipping(distance);

    Render::RenderSceneController::instance().setCamera(m_cameraState);

    update();
}

// =============================================================================
// GLViewportRenderer Implementation
// =============================================================================

GLViewportRenderer::GLViewportRenderer(const GLViewport* viewport)
    : m_viewport(viewport), m_sceneRenderer(std::make_unique<Render::SceneRenderer>()) {
    LOG_TRACE("GLViewportRenderer created");
}

GLViewportRenderer::~GLViewportRenderer() {
    m_sceneRenderer->cleanup();
    LOG_TRACE("GLViewportRenderer destroyed");
}

QOpenGLFramebufferObject* GLViewportRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4); // Enable MSAA
    m_viewportSize = size;
    m_sceneRenderer->setViewportSize(size);
    return new QOpenGLFramebufferObject(size, format);
}

void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = qobject_cast<GLViewport*>(item);
    if(!viewport) {
        return;
    }

    m_cameraState = viewport->cameraState();

    // Check if render data changed using version number
    const auto& new_render_data = viewport->renderData();
    if(new_render_data.m_version != m_renderData.m_version) {
        LOG_DEBUG("GLViewportRenderer: Render data changed, version {} -> {}, uploading {} meshes",
                  m_renderData.m_version, new_render_data.m_version, new_render_data.meshCount());
        m_renderData = new_render_data;
        m_needsDataUpload = true;
    }
}

void GLViewportRenderer::render() {
    if(!m_sceneRenderer->isInitialized()) {
        m_sceneRenderer->initialize();
    }

    if(m_needsDataUpload) {
        m_sceneRenderer->uploadMeshData(m_renderData);
        m_needsDataUpload = false;
    }

    // Calculate matrices
    const float aspect_ratio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    const QMatrix4x4 projection = m_cameraState.projectionMatrix(aspect_ratio);
    const QMatrix4x4 view = m_cameraState.viewMatrix();

    // Delegate rendering to SceneRenderer
    m_sceneRenderer->render(m_cameraState.m_position, view, projection);
}

} // namespace OpenGeoLab::App
