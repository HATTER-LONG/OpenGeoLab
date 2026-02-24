#include "app/select_manager_service.hpp"
#include "render/render_select_manager.hpp"
#include "util/logger.hpp"
#include <QtCore/QMetaObject>

namespace OpenGeoLab::App {

namespace {

QString convertPickResultTypeToString(const Render::PickResult& result) {
    if(result.m_type == Render::PickEntityType::MeshElement ||
       result.m_type == Render::PickEntityType::MeshLine) {
        const auto mesh_elem_type_opt =
            meshElementTypeToString(static_cast<Mesh::MeshElementType>(result.m_meshElementType));
        if(mesh_elem_type_opt.has_value()) {
            return QString::fromStdString(mesh_elem_type_opt.value());
        }
        return "Invalid";
    }
    if(result.m_type == Render::PickEntityType::MeshNode) {
        return "MeshNode";
    }
    const auto geom_type_opt = Render::toGeometryType(result.m_type);
    if(geom_type_opt.has_value()) {
        const auto geom_type_str_opt = Geometry::entityTypeToString(geom_type_opt.value());
        if(geom_type_str_opt.has_value()) {
            return QString::fromStdString(geom_type_str_opt.value());
        }
    }
    return "Invalid";
}

std::variant<Geometry::EntityType, Mesh::MeshElementType, Mesh::EntityType>
convertStringToPickEntityType(const QString& type_str) {
    const auto geom_type_opt = Geometry::entityTypeFromString(type_str.toStdString());
    if(geom_type_opt.has_value()) {
        return geom_type_opt.value();
    }

    const auto mesh_elem_type_opt = Mesh::meshElementTypeFromString(type_str.toStdString());
    if(mesh_elem_type_opt.has_value()) {
        return mesh_elem_type_opt.value();
    }

    if(type_str.compare("MeshNode", Qt::CaseInsensitive) == 0) {
        return Mesh::EntityType::Node;
    }

    throw std::invalid_argument("Unknown entity type string: " + type_str.toStdString());
}
} // namespace

SelectManagerService::SelectManagerService(QObject* parent) : QObject(parent) {
    m_selectSettingsConn = Render::RenderSelectManager::instance().subscribePickSettingsChanged(
        [this](Render::PickMask types) {
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
                        emit entitySelected(static_cast<uint32_t>(pr.m_uid), type_str);
                        break;
                    }
                    case Render::SelectionChangeAction::Removed: {
                        const auto type_str = convertPickResultTypeToString(pr);
                        emit entityRemoved(static_cast<uint32_t>(pr.m_uid), type_str);
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
    select.setPickTypes(static_cast<Render::PickMask>(select_types));
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

void SelectManagerService::selectEntity(uint32_t entity_uid, const QString& entity_type) {
    try {
        auto entity_type_enum = convertStringToPickEntityType(entity_type);
        if(std::holds_alternative<Mesh::MeshElementType>(entity_type_enum)) {
            const auto mesh_elem_type = std::get<Mesh::MeshElementType>(entity_type_enum);
            Render::RenderSelectManager::instance().addSelection(
                static_cast<Mesh::MeshElementUID>(entity_uid), mesh_elem_type);
            return;
        }

        if(std::holds_alternative<Mesh::EntityType>(entity_type_enum) &&
           std::get<Mesh::EntityType>(entity_type_enum) == Mesh::EntityType::Node) {
            Render::RenderSelectManager::instance().addSelection(
                static_cast<Mesh::MeshNodeId>(entity_uid));
            return;
        }

        if(std::holds_alternative<Geometry::EntityType>(entity_type_enum)) {
            const auto geom_type = std::get<Geometry::EntityType>(entity_type_enum);
            Render::RenderSelectManager::instance().addSelection(
                static_cast<Geometry::EntityUID>(entity_uid), geom_type);
            return;
        }

    } catch(...) {
        LOG_ERROR("SelectManagerService::selectEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
    return;
}

void SelectManagerService::removeEntity(uint32_t entity_uid, const QString& entity_type) {
    try {
        auto entity_type_enum = convertStringToPickEntityType(entity_type);
        if(std::holds_alternative<Mesh::MeshElementType>(entity_type_enum)) {
            const auto mesh_elem_type = std::get<Mesh::MeshElementType>(entity_type_enum);
            Render::RenderSelectManager::instance().removeSelection(
                static_cast<Mesh::MeshElementUID>(entity_uid), mesh_elem_type);
            return;
        }

        if(std::holds_alternative<Mesh::EntityType>(entity_type_enum) &&
           std::get<Mesh::EntityType>(entity_type_enum) == Mesh::EntityType::Node) {
            Render::RenderSelectManager::instance().removeSelection(
                static_cast<Mesh::MeshNodeId>(entity_uid));
            return;
        }

        if(std::holds_alternative<Geometry::EntityType>(entity_type_enum)) {
            const auto geom_type = std::get<Geometry::EntityType>(entity_type_enum);
            Render::RenderSelectManager::instance().removeSelection(
                static_cast<Geometry::EntityUID>(entity_uid), geom_type);
            return;
        }

    } catch(...) {
        LOG_ERROR("SelectManagerService::removeEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
    return;
}

QVector<QPair<uint32_t, QString>> SelectManagerService::currentSelections() const {
    QVector<QPair<uint32_t, QString>> out;
    const auto sels = Render::RenderSelectManager::instance().selections();
    for(const auto& pr : sels) {
        out.append({static_cast<uint32_t>(pr.m_uid), convertPickResultTypeToString(pr)});
    }
    return out;
}

} // namespace OpenGeoLab::App