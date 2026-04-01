/**
 * @file selection_service.cpp
 * @brief SelectionService implementation
 */

#include <opengeolab/app/selection_service.hpp>

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/pick_mask.hpp>
#include <opengeolab/scene/selection_state.hpp>

#include <QMetaObject>
#include <QVariantMap>

namespace OpenGeoLab::App {

SelectionService::SelectionService(QObject* parent) : QObject(parent) {}

SelectionService::~SelectionService() { disconnectSignals(); }

void SelectionService::setSelectionState(Scene::SelectionState* state) {
    if(m_state == state) {
        return;
    }
    disconnectSignals();
    m_state = state;
    if(m_state != nullptr) {
        connectSignals();
    }
}

bool SelectionService::pickEnabled() const {
    return m_state != nullptr && m_state->pickEnabled();
}

void SelectionService::setPickEnabled(bool enabled) {
    if(m_state == nullptr) {
        return;
    }
    m_state->setPickEnabled(enabled);
}

int SelectionService::pickMask() const {
    if(m_state == nullptr) {
        return 0;
    }
    return static_cast<int>(m_state->pickMask());
}

void SelectionService::setPickMask(int mask) {
    if(m_state == nullptr) {
        return;
    }
    m_state->setPickMask(static_cast<Core::PickMask>(mask));
}

QVariantList SelectionService::selections() const {
    QVariantList result;
    if(m_state == nullptr) {
        return result;
    }
    for(const auto& entity : m_state->selections()) {
        QVariantMap item;
        item["shapeId"] = static_cast<int>(entity.shapeId);
        item["entityType"] = static_cast<int>(entity.entityType);
        item["localId"] = static_cast<int>(entity.localId);
        result.append(item);
    }
    return result;
}

void SelectionService::activatePickMode(int mask) {
    if(m_state == nullptr) {
        return;
    }
    m_state->setPickMask(static_cast<Core::PickMask>(mask));
    m_state->setPickEnabled(true);
}

void SelectionService::deactivatePickMode() {
    if(m_state == nullptr) {
        return;
    }
    m_state->setPickEnabled(false);
}

void SelectionService::clearSelection() {
    if(m_state == nullptr) {
        return;
    }
    m_state->clearSelection();
}

void SelectionService::removeSelection(int shape_id, int entity_type, int local_id) {
    if(m_state == nullptr) {
        return;
    }
    const Core::EntityRef entity{static_cast<uint32_t>(shape_id),
                                 static_cast<Core::EntityType>(entity_type),
                                 static_cast<uint32_t>(local_id)};
    m_state->removeSelection(entity);
}

void SelectionService::connectSignals() {
    if(m_state == nullptr) {
        return;
    }

    m_connections.push_back(m_state->entitySelected.connect([this](const Core::EntityRef& entity) {
        QMetaObject::invokeMethod(
            this,
            [this, entity]() {
                Q_EMIT entitySelected(static_cast<int>(entity.shapeId),
                                      static_cast<int>(entity.entityType),
                                      static_cast<int>(entity.localId));
                Q_EMIT selectionChanged();
            },
            Qt::QueuedConnection);
    }));

    m_connections.push_back(
        m_state->entityDeselected.connect([this](const Core::EntityRef& entity) {
            QMetaObject::invokeMethod(
                this,
                [this, entity]() {
                    Q_EMIT entityDeselected(static_cast<int>(entity.shapeId),
                                            static_cast<int>(entity.entityType),
                                            static_cast<int>(entity.localId));
                    Q_EMIT selectionChanged();
                },
                Qt::QueuedConnection);
        }));

    m_connections.push_back(m_state->selectionCleared.connect([this]() {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                Q_EMIT selectionCleared();
                Q_EMIT selectionChanged();
            },
            Qt::QueuedConnection);
    }));

    m_connections.push_back(
        m_state->hoverChanged.connect([this](const std::optional<Core::EntityRef>& entity) {
            QMetaObject::invokeMethod(
                this,
                [this, entity]() {
                    if(entity.has_value()) {
                        Q_EMIT hoverChanged(static_cast<int>(entity->shapeId),
                                            static_cast<int>(entity->entityType),
                                            static_cast<int>(entity->localId));
                    } else {
                        Q_EMIT hoverChanged(-1, -1, -1);
                    }
                },
                Qt::QueuedConnection);
        }));

    m_connections.push_back(m_state->pickConfigChanged.connect([this]() {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                Q_EMIT pickEnabledChanged();
                Q_EMIT pickMaskChanged();
            },
            Qt::QueuedConnection);
    }));
}

void SelectionService::disconnectSignals() { m_connections.clear(); }

} // namespace OpenGeoLab::App
