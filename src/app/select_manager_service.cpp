/**
 * @file select_manager_service.cpp
 * @brief QML-exposed selection manager service bridging render SelectManager to QML
 */
#include "app/select_manager_service.hpp"
#include "render/render_types.hpp"
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
                            QString::fromUtf8(Render::renderEntityTypeToString(pr.m_type));
                        emit entitySelected(pr.m_uid56, type_str);
                        break;
                    }
                    case Render::SelectManager::SelectionChangeAction::Removed: {
                        const auto type_str =
                            QString::fromUtf8(Render::renderEntityTypeToString(pr.m_type));
                        emit entityRemoved(pr.m_uid56, type_str);
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

void SelectManagerService::selectEntity(uint64_t entity_uid, const QString& entity_type) {
    const auto type = Render::renderEntityTypeFromString(entity_type.toStdString());
    if(type == Render::RenderEntityType::None) {
        LOG_ERROR("SelectManagerService::selectEntity: Invalid entity type '{}'",
                  entity_type.toStdString());
        return;
    }
    Render::SelectManager::instance().addSelection(entity_uid, type);
}

void SelectManagerService::removeEntity(uint64_t entity_uid, const QString& entity_type) {
    const auto type = Render::renderEntityTypeFromString(entity_type.toStdString());
    if(type == Render::RenderEntityType::None) {
        LOG_ERROR("SelectManagerService::removeEntity: Invalid entity type '{}'",
                  entity_type.toStdString());
        return;
    }
    Render::SelectManager::instance().removeSelection(entity_uid, type);
}

bool SelectManagerService::isEntitySelected(uint64_t entity_uid, const QString& entity_type) const {
    const auto type = Render::renderEntityTypeFromString(entity_type.toStdString());
    if(type == Render::RenderEntityType::None) {
        LOG_ERROR("SelectManagerService::isEntitySelected: Invalid entity type '{}'",
                  entity_type.toStdString());
        return false;
    }
    return Render::SelectManager::instance().containsSelection(entity_uid, type);
}

QVector<QPair<uint64_t, QString>> SelectManagerService::currentSelections() const {
    QVector<QPair<uint64_t, QString>> out;
    const auto sels = Render::SelectManager::instance().selections();
    for(const auto& pr : sels) {
        out.append({pr.m_uid56, QString::fromUtf8(Render::renderEntityTypeToString(pr.m_type))});
    }
    return out;
}

} // namespace OpenGeoLab::App
