#include <opengeolab/render/viewport_item.hpp>
#include <opengeolab/render/viewport_renderer.hpp>

#include <QMouseEvent>
#include <QRectF>
#include <QWheelEvent>

namespace OpenGeoLab::Render {

ViewportItem::ViewportItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsInputMethod);
    setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer* ViewportItem::createRenderer() const {
    return new ViewportRenderer();
}

void ViewportItem::setSceneGraph(Scene::SceneGraph* graph) {
    m_sceneGraph = graph;
    emit sceneChanged();
    update();
}

Scene::SceneGraph* ViewportItem::sceneGraph() const { return m_sceneGraph; }

Camera& ViewportItem::camera() { return m_camera; }

const Camera& ViewportItem::camera() const { return m_camera; }

void ViewportItem::fitAll() {
    if(m_sceneGraph == nullptr) {
        return;
    }

    auto bounds = m_sceneGraph->sceneBounds();
    if(!bounds.isValid()) {
        return;
    }

    m_camera.fitToBoundingBox(bounds.center(), bounds.radius());
    m_trackball.syncFromCamera(m_camera);
    update();
}

void ViewportItem::setFrontView() {
    m_camera.setFrontView();
    m_trackball.syncFromCamera(m_camera);
    update();
}

void ViewportItem::setBackView() {
    m_camera.setBackView();
    m_trackball.syncFromCamera(m_camera);
    update();
}

void ViewportItem::setLeftView() {
    m_camera.setLeftView();
    m_trackball.syncFromCamera(m_camera);
    update();
}

void ViewportItem::setRightView() {
    m_camera.setRightView();
    m_trackball.syncFromCamera(m_camera);
    update();
}

void ViewportItem::setTopView() {
    m_camera.setTopView();
    m_trackball.syncFromCamera(m_camera);
    update();
}

void ViewportItem::setBottomView() {
    m_camera.setBottomView();
    m_trackball.syncFromCamera(m_camera);
    update();
}

// ── Mouse interaction ───────────────────────────────────────────────────────

void ViewportItem::mousePressEvent(QMouseEvent* event) {
    auto pos = event->position();
    float x = static_cast<float>(pos.x());
    float y = static_cast<float>(pos.y());

    TrackballController::Mode mode = TrackballController::Mode::None;
    Qt::MouseButton btn = event->button();
    Qt::KeyboardModifiers mods = event->modifiers();

    if(btn == Qt::LeftButton && (mods & Qt::ControlModifier)) {
        mode = TrackballController::Mode::Orbit;
    } else if(btn == Qt::LeftButton && (mods & Qt::ShiftModifier)) {
        mode = TrackballController::Mode::Pan;
    } else if(btn == Qt::MiddleButton) {
        mode = TrackballController::Mode::Pan;
    } else if(btn == Qt::RightButton) {
        mode = TrackballController::Mode::Zoom;
    }

    if(mode != TrackballController::Mode::None) {
        m_trackball.begin(x, y, mode, m_camera);
    }

    event->accept();
}

void ViewportItem::mouseMoveEvent(QMouseEvent* event) {
    if(!m_trackball.isActive()) {
        return;
    }

    auto pos = event->position();
    m_trackball.update(static_cast<float>(pos.x()), static_cast<float>(pos.y()), m_camera);
    update();
    event->accept();
}

void ViewportItem::mouseReleaseEvent(QMouseEvent* event) {
    m_trackball.end();
    event->accept();
}

void ViewportItem::wheelEvent(QWheelEvent* event) {
    float steps = static_cast<float>(event->angleDelta().y()) / 120.f;
    m_trackball.syncFromCamera(m_camera);
    m_trackball.wheelZoom(steps, m_camera);
    update();
    event->accept();
}

void ViewportItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickFramebufferObject::geometryChange(newGeometry, oldGeometry);
    m_trackball.setViewportSize(static_cast<float>(newGeometry.width()),
                                static_cast<float>(newGeometry.height()));
    update();
}

} // namespace OpenGeoLab::Render
