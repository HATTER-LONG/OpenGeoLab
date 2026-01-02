/**
 * @file viewport_item.cpp
 * @brief Implementation of QML viewport item for OpenGL rendering
 */

#include "render/viewport_item.hpp"
#include "geometry/geometry_model.hpp"
#include "render/geometry_converter.hpp"
#include "util/logger.hpp"

#include <QQuickWindow>
#include <QSGSimpleRectNode>

namespace OpenGeoLab {
namespace Render {

ViewportItem::ViewportItem(QQuickItem* parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);

    connect(this, &QQuickItem::windowChanged, this, &ViewportItem::handleWindowChanged);

    // Register for geometry changes
    m_geometryCallbackId = Geometry::GeometryStore::instance().registerChangeCallback(
        [this]() { QMetaObject::invokeMethod(this, "onGeometryChanged", Qt::QueuedConnection); });
}

ViewportItem::~ViewportItem() {
    Geometry::GeometryStore::instance().unregisterChangeCallback(m_geometryCallbackId);
}

bool ViewportItem::hasGeometry() const {
    return m_renderer && m_renderer->renderGeometry() && !m_renderer->renderGeometry()->isEmpty();
}

void ViewportItem::setSelectionMode(int mode) {
    if(mode < 0 || mode > static_cast<int>(SelectionMode::Part)) {
        mode = 0;
    }
    if(m_selectionMode != static_cast<SelectionMode>(mode)) {
        m_selectionMode = static_cast<SelectionMode>(mode);
        emit selectionModeChanged();
    }
}

void ViewportItem::fitToView() {
    if(m_renderer) {
        m_renderer->fitToView();
        if(window()) {
            window()->update();
        }
    }
}

void ViewportItem::resetView() {
    if(m_renderer) {
        m_renderer->camera()->reset();
        m_renderer->resetModelRotation();
        if(window()) {
            window()->update();
        }
    }
}

void ViewportItem::clearSelection() {
    if(m_selectedId != 0) {
        m_selectedId = 0;
        emit selectionChanged(0, 0);
    }
}

void ViewportItem::handleWindowChanged(QQuickWindow* window) {
    if(window) {
        connect(window, &QQuickWindow::beforeSynchronizing, this, &ViewportItem::initializeRenderer,
                Qt::DirectConnection);
        connect(window, &QQuickWindow::beforeRendering, this, &ViewportItem::handleBeforeRendering,
                Qt::DirectConnection);
        connect(window, &QQuickWindow::beforeRenderPassRecording, this,
                &ViewportItem::handleBeforeRenderPassRecording, Qt::DirectConnection);

        window->setColor(Qt::transparent);
    }
}

void ViewportItem::initializeRenderer() {
    if(!m_renderer) {
        m_renderer = std::make_unique<OpenGLRenderer>();
    }

    m_renderer->setWindow(window());
    m_renderer->init();

    // Update geometry if available
    updateGeometry();
}

void ViewportItem::handleBeforeRendering() {
    // Update viewport size
    if(m_renderer) {
        QPointF pos = mapToScene(QPointF(0, 0));
        m_renderer->setViewportOffset(QPoint(
            static_cast<int>(pos.x()), static_cast<int>(window()->height() - pos.y() - height())));
        m_renderer->setViewportSize(QSize(static_cast<int>(width()), static_cast<int>(height())));
    }
}

void ViewportItem::handleBeforeRenderPassRecording() {
    if(m_renderer) {
        m_renderer->paint();
    }
}

void ViewportItem::onGeometryChanged() {
    updateGeometry();
    emit geometryChanged();
    if(window()) {
        window()->update();
    }
}

void ViewportItem::updateGeometry() {
    if(!m_renderer) {
        return;
    }

    auto model = Geometry::GeometryStore::instance().getModel();
    if(model && !model->isEmpty()) {
        auto renderGeometry = GeometryConverter::convert(*model);
        m_renderer->setRenderGeometry(renderGeometry);
        m_renderer->fitToView();
        LOG_INFO("ViewportItem: Geometry updated, {} vertices", renderGeometry->vertices.size());
    } else {
        m_renderer->setRenderGeometry(nullptr);
    }
}

void ViewportItem::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->pos();

    if(event->modifiers() & Qt::ControlModifier && event->button() == Qt::LeftButton) {
        // Ctrl + Left click: start rotation
        m_rotating = true;
        if(m_renderer) {
            m_renderer->trackball()->begin(event->pos().x(), event->pos().y());
        }
        event->accept();
    } else if(event->modifiers() & Qt::ShiftModifier && event->button() == Qt::LeftButton) {
        // Shift + Left click: start panning
        m_panning = true;
        event->accept();
    } else if(m_selectionMode != SelectionMode::None && event->button() == Qt::LeftButton) {
        // Selection mode: perform picking
        m_pendingPick = true;
        m_pickPosition = event->pos();
        event->accept();
    } else {
        event->ignore();
    }
}

void ViewportItem::mouseReleaseEvent(QMouseEvent* event) {
    if(m_pendingPick && m_selectionMode != SelectionMode::None) {
        performPicking(m_pickPosition.x(), m_pickPosition.y());
        m_pendingPick = false;
    }

    m_rotating = false;
    m_panning = false;
    event->accept();
}

void ViewportItem::mouseMoveEvent(QMouseEvent* event) {
    if(!m_renderer) {
        event->ignore();
        return;
    }

    QPoint delta = event->pos() - m_lastMousePos;

    if(m_rotating) {
        // Trackball rotation
        QQuaternion rotation = m_renderer->trackball()->rotate(event->pos().x(), event->pos().y());
        m_renderer->rotateModelByQuaternion(rotation);

        if(window()) {
            window()->update();
        }
        event->accept();
    } else if(m_panning) {
        // Camera panning
        m_renderer->camera()->panByPixel(m_lastMousePos.x(), m_lastMousePos.y(), event->pos().x(),
                                         event->pos().y());

        if(window()) {
            window()->update();
        }
        event->accept();
    } else {
        event->ignore();
    }

    m_lastMousePos = event->pos();
}

void ViewportItem::wheelEvent(QWheelEvent* event) {
    if(!m_renderer) {
        event->ignore();
        return;
    }

    // Ctrl + wheel: zoom
    if(event->modifiers() & Qt::ControlModifier) {
        float delta = event->angleDelta().y();
        float factor = 1.0f + delta / 1200.0f;
        m_renderer->camera()->zoom(factor);

        if(window()) {
            window()->update();
        }
        event->accept();
    } else {
        event->ignore();
    }
}

QSGNode* ViewportItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    // We don't use scene graph nodes for OpenGL rendering
    // The rendering is done directly in beforeRenderPassRecording
    return oldNode;
}

void ViewportItem::performPicking(int x, int y) {
    // TODO: Implement proper picking using ray casting or color picking
    // For now, just emit a placeholder selection
    LOG_DEBUG("Picking at ({}, {}), mode: {}", x, y, static_cast<int>(m_selectionMode));

    // Placeholder: emit selection changed with dummy ID
    // Real implementation would use ray casting against geometry
    m_selectedId = 1; // Dummy ID
    emit selectionChanged(m_selectedId, static_cast<int>(m_selectionMode));
}

} // namespace Render
} // namespace OpenGeoLab
