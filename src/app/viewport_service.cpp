/**
 * @file viewport_service.cpp
 * @brief ViewportService implementation
 */

#include "app/viewport_service.hpp"
#include "render/render_scene_controller.hpp"
#include "render/select_manager.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::App {

ViewportService::ViewportService(QObject* parent) : QObject(parent) {
    LOG_TRACE("ViewportService created");
}

ViewportService* ViewportService::create(QQmlEngine* engine, QJSEngine* script_engine) {
    Q_UNUSED(engine)
    Q_UNUSED(script_engine)
    static ViewportService instance;
    return &instance;
}

QString ViewportService::activeViewportId() const { return m_activeViewportId; }

void ViewportService::setActiveViewportId(const QString& id) {
    if(m_activeViewportId != id) {
        m_activeViewportId = id;
        emit activeViewportIdChanged();
        LOG_TRACE("ViewportService: Active viewport changed to '{}'", id.toStdString());
    }
}

bool ViewportService::isPickEnabled() const {
    return Render::SelectManager::instance().isPickEnabled();
}

void ViewportService::fitToScene() { Render::RenderSceneController::instance().fitToScene(true); }

void ViewportService::setStandardView(const QString& viewName) {
    auto& ctrl = Render::RenderSceneController::instance();
    if(viewName == "front") {
        ctrl.setFrontView(true);
    } else if(viewName == "back") {
        ctrl.setBackView(true);
    } else if(viewName == "top") {
        ctrl.setTopView(true);
    } else if(viewName == "bottom") {
        ctrl.setBottomView(true);
    } else if(viewName == "left") {
        ctrl.setLeftView(true);
    } else if(viewName == "right") {
        ctrl.setRightView(true);
    } else {
        LOG_WARN("ViewportService: Unknown view name '{}'", viewName.toStdString());
    }
}

void ViewportService::resetCamera() { Render::RenderSceneController::instance().resetCamera(true); }

void ViewportService::notifyPick(int uid, int entityType, int action) {
    emit pickResult(uid, entityType, action);
}

void ViewportService::notifyHover(int uid, int entityType) { emit hoverChanged(uid, entityType); }

void ViewportService::setPartGeometryVisible(int partUid, bool visible) {
    Render::RenderSceneController::instance().setPartGeometryVisible(
        static_cast<Geometry::EntityUID>(partUid), visible);
}

void ViewportService::setPartMeshVisible(int partUid, bool visible) {
    Render::RenderSceneController::instance().setPartMeshVisible(
        static_cast<Geometry::EntityUID>(partUid), visible);
}

bool ViewportService::isPartGeometryVisible(int partUid) const {
    return Render::RenderSceneController::instance().isPartGeometryVisible(
        static_cast<Geometry::EntityUID>(partUid));
}

bool ViewportService::isPartMeshVisible(int partUid) const {
    return Render::RenderSceneController::instance().isPartMeshVisible(
        static_cast<Geometry::EntityUID>(partUid));
}

} // namespace OpenGeoLab::App
