#include "app/select_manager_service.hpp"
#include "render/select_manager.hpp"
#include "util/logger.hpp"
#include <QtCore/QMetaObject>

namespace OpenGeoLab::App {

SelectManagerService::SelectManagerService(QObject* parent) : QObject(parent) {
    m_selectSettingsConn = Render::SelectManager::instance().subscribePickSettingsChanged(
        [this](Render::SelectManager::PickTypes types) {
            QMetaObject::invokeMethod(
                this, [this, types]() { emit selectModeChanged(static_cast<uint32_t>(types)); },
                Qt::QueuedConnection);
        });

    m_selectionConn = Render::SelectManager::instance().subscribeSelectionChanged(
        [this](Render::SelectManager::PickResult pr,
               Render::SelectManager::SelectionChangeAction action) {
            QMetaObject::invokeMethod(
                this,
                [this, pr, action]() {
                    switch(action) {
                    case Render::SelectManager::SelectionChangeAction::Added: {
                        const auto type_str =
                            QString::fromStdString(Geometry::entityTypeToString(pr.m_type));
                        emit entitySelected(static_cast<uint32_t>(pr.m_uid), type_str);
                        break;
                    }
                    case Render::SelectManager::SelectionChangeAction::Removed: {
                        const auto type_str =
                            QString::fromStdString(Geometry::entityTypeToString(pr.m_type));
                        emit entityRemoved(static_cast<uint32_t>(pr.m_uid), type_str);
                        break;
                    }
                    case Render::SelectManager::SelectionChangeAction::Cleared:
                        emit selectionCleared();
                        break;
                    }
                },
                Qt::QueuedConnection);
        });
    m_selectionEnable =
        Render::SelectManager::instance().subscribePickEnabledChanged([this](bool enabled) {
            QMetaObject::invokeMethod(
                this, [this, enabled]() { emit selectModeActivated(enabled); },
                Qt::QueuedConnection);
        });
}
SelectManagerService::~SelectManagerService() = default;

void SelectManagerService::activateSelectMode(uint32_t select_types) {
    auto& sm = Render::SelectManager::instance();
    sm.setPickEnabled(true);
    sm.setPickTypes(static_cast<Render::SelectManager::PickTypes>(select_types));
}

void SelectManagerService::deactivateSelectMode() {
    Render::SelectManager::instance().setPickEnabled(false);
}

bool SelectManagerService::isSelectEnabled() const {
    return Render::SelectManager::instance().isPickEnabled();
}

void SelectManagerService::clearSelection() { Render::SelectManager::instance().clearSelections(); }

void SelectManagerService::selectEntity(uint32_t entity_uid, const QString& entity_type) {
    try {
        const Geometry::EntityType t = Geometry::entityTypeFromString(entity_type.toStdString());
        Render::SelectManager::instance().addSelection(static_cast<Geometry::EntityUID>(entity_uid),
                                                       t);
    } catch(...) {
        LOG_ERROR("SelectManagerService::selectEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
}

void SelectManagerService::removeEntity(uint32_t entity_uid, const QString& entity_type) {
    try {
        const Geometry::EntityType t = Geometry::entityTypeFromString(entity_type.toStdString());
        Render::SelectManager::instance().removeSelection(
            static_cast<Geometry::EntityUID>(entity_uid), t);
    } catch(...) {
        LOG_ERROR("SelectManagerService::removeEntity: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return;
    }
}

bool SelectManagerService::isEntitySelected(uint32_t entity_uid, const QString& entity_type) const {
    try {
        const Geometry::EntityType t = Geometry::entityTypeFromString(entity_type.toStdString());
        return Render::SelectManager::instance().containsSelection(
            static_cast<Geometry::EntityUID>(entity_uid), t);
    } catch(...) {
        LOG_ERROR("SelectManagerService::isEntitySelected: Invalid entity '{}:{}'",
                  entity_type.toStdString(), entity_uid);
        return false;
    }
}

QVector<QPair<uint32_t, QString>> SelectManagerService::currentSelections() const {
    QVector<QPair<uint32_t, QString>> out;
    const auto sels = Render::SelectManager::instance().selections();
    for(const auto& pr : sels) {
        out.append({static_cast<uint32_t>(pr.m_uid),
                    QString::fromStdString(Geometry::entityTypeToString(pr.m_type))});
    }
    return out;
}

} // namespace OpenGeoLab::App