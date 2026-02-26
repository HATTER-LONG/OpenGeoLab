/**
 * @file select_manager_service.cpp
 * @brief SelectManagerService — bridges RenderSelectManager to QML for
 *        pick-mode and entity-filter configuration.
 */

#include "app/select_manager_service.hpp"
#include "render/render_select_manager.hpp"
#include "util/logger.hpp"
#include <QtCore/QMetaObject>

namespace OpenGeoLab::App {

namespace {

QString convertPickResultTypeToString(const Render::PickResult& result) {
    if(Render::isMeshDomain(result.m_type)) {
        const auto mesh_elem_type_opt =
            meshElementTypeToString(Render::toMeshElementType(result.m_type));
        if(mesh_elem_type_opt.has_value()) {
            return QString::fromStdString(mesh_elem_type_opt.value());
        }
    } else if(Render::isGeometryDomain(result.m_type)) {
        const auto geom_type_opt =
            Geometry::entityTypeToString(Render::toGeometryType(result.m_type));
        if(geom_type_opt.has_value()) {
            return QString::fromStdString(geom_type_opt.value());
        }
    }
    return "Invalid";
}

Render::RenderEntityType convertStringToRenderEntityType(const QString& type_str) {
    const auto geom_type_opt = Geometry::entityTypeFromString(type_str.toStdString());
    if(geom_type_opt.has_value()) {
        return Render::toRenderEntityType(geom_type_opt.value());
    }

    const auto mesh_elem_type_opt = Mesh::meshElementTypeFromString(type_str.toStdString());
    if(mesh_elem_type_opt.has_value()) {
        return Render::toRenderEntityType(mesh_elem_type_opt.value());
    }

    throw std::invalid_argument("Unknown entity type string: " + type_str.toStdString());
}
} // namespace

SelectManagerService::SelectManagerService(QObject* parent) : QObject(parent) {
    m_selectSettingsConn = Render::RenderSelectManager::instance().subscribePickSettingsChanged(
        [this](Render::RenderEntityTypeMask types) {
            QMetaObject::invokeMethod(
                this, [this, types]() { emit selectModeChanged(static_cast<uint32_t>(types)); },
                Qt::QueuedConnection);
        });

    m_selectionConn = Render::RenderSelectManager::instance().subscribeSelectionChanged(
        [this](Render::PickResult pr, Render::SelectionChangeAction action) {
            QMetaObject::invokeMethod(
                this,
                [this, pr, action]() {
                    switch(action) {
                    case Render::SelectionChangeAction::Added: {
                        const auto type_str = convertPickResultTypeToString(pr);
                        emit entitySelected(static_cast<uint64_t>(pr.m_uid), type_str);
                        break;
                    }
                    case Render::SelectionChangeAction::Removed: {
                        const auto type_str = convertPickResultTypeToString(pr);
                        emit entityRemoved(static_cast<uint64_t>(pr.m_uid), type_str);
                        break;
                    }
                    case Render::SelectionChangeAction::Cleared:
                        emit selectionCleared();
                        break;
                    }
                },
                Qt::QueuedConnection);
        });
    m_selectionEnable =
        Render::RenderSelectManager::instance().subscribePickEnabledChanged([this](bool enabled) {
            QMetaObject::invokeMethod(
                this, [this, enabled]() { emit selectModeActivated(enabled); },
                Qt::QueuedConnection);
        });
}
SelectManagerService::~SelectManagerService() = default;

void SelectManagerService::activateSelectMode(uint32_t select_types) {
    auto& select = Render::RenderSelectManager::instance();
    select.setPickEnabled(true);
    select.setPickTypes(static_cast<Render::RenderEntityTypeMask>(select_types));
}

void SelectManagerService::deactivateSelectMode() {
    Render::RenderSelectManager::instance().setPickEnabled(false);
}

bool SelectManagerService::isSelectEnabled() const {
    return Render::RenderSelectManager::instance().isPickEnabled();
}

void SelectManagerService::clearSelection() {
    Render::RenderSelectManager::instance().clearSelection();
}

void SelectManagerService::selectEntity(uint64_t entity_uid, const QString& entity_type) {
    try {
        auto entity_type_enum = convertStringToRenderEntityType(entity_type);

        Render::RenderSelectManager::instance().addSelection(
            static_cast<Mesh::MeshElementUID>(entity_uid), entity_type_enum);
    } catch(...) {
        LOG_ERROR("SelectManagerService::selectEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
    return;
}

void SelectManagerService::removeEntity(uint64_t entity_uid, const QString& entity_type) {
    try {
        auto entity_type_enum = convertStringToRenderEntityType(entity_type);

        Render::RenderSelectManager::instance().removeSelection(
            static_cast<Mesh::MeshElementUID>(entity_uid), entity_type_enum);

    } catch(...) {
        LOG_ERROR("SelectManagerService::removeEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
    return;
}

QVector<QPair<uint64_t, QString>> SelectManagerService::currentSelections() const {
    QVector<QPair<uint64_t, QString>> out;
    const auto sels = Render::RenderSelectManager::instance().selections();
    for(const auto& pr : sels) {
        out.append({static_cast<uint64_t>(pr.m_uid), convertPickResultTypeToString(pr)});
    }
    return out;
}

} // namespace OpenGeoLab::App