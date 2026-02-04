#include "app/pick_manager.hpp"
#include "app/opengl_viewport.hpp"
#include "geometry/geometry_types.hpp"
#include <QMetaObject>
#include <QVariantMap>

namespace OpenGeoLab::App {

PickManager::PickManager(QObject* parent) : QObject(parent) {}

PickManager::~PickManager() { setViewport(nullptr); }

QObject* PickManager::viewport() const { return m_viewport.data(); }

void PickManager::setViewport(QObject* vp) {
    if(m_viewport.data() == vp) {
        return;
    }

    if(m_viewport) {
        QObject::disconnect(m_connPicked);
        QObject::disconnect(m_connCancelled);
        m_connPicked = {};
        m_connCancelled = {};
    }

    m_viewport = vp;

    if(m_viewport) {
        m_connPicked = QObject::connect(m_viewport, SIGNAL(entityPicked(QString, int)), this,
                                        SLOT(onViewportEntityPicked(QString, int)));
        m_connCancelled = QObject::connect(m_viewport, SIGNAL(pickCancelled()), this,
                                           SLOT(onViewportPickCancelled()));
        // Connect unpick signal for right-click removal
        QObject::connect(m_viewport, SIGNAL(entityUnpicked(QString, int)), this,
                         SLOT(onViewportEntityUnpicked(QString, int)));
    }

    applyViewportState();
    emit viewportChanged();
}

QString PickManager::activeConsumerKey() const { return m_activeConsumerKey; }

void PickManager::setActiveConsumer(const QString& key) {
    const QString new_key = key;
    if(m_activeConsumerKey == new_key) {
        return;
    }
    m_activeConsumerKey = new_key;
    emit activeConsumerKeyChanged();

    setContext(m_activeConsumerKey);
}

QString PickManager::contextKey() const { return m_contextKey; }

void PickManager::setContext(const QString& key) {
    const QString new_key = key;
    if(m_contextKey == new_key) {
        return;
    }

    m_contextKey = new_key;
    emit contextKeyChanged();

    loadSelectionForContext(m_contextKey);
    emit selectionChanged(effectiveContextKey(), QVariant::fromValue(m_selectedEntities));
}

void PickManager::clearActiveConsumer() {
    if(m_activeConsumerKey.isEmpty()) {
        return;
    }
    m_activeConsumerKey.clear();
    emit activeConsumerKeyChanged();
}

QString PickManager::selectedType() const { return m_selectedType; }
bool PickManager::pickModeActive() const { return m_pickModeActive; }

QVariantList PickManager::selectedEntities() const { return m_selectedEntities; }

bool PickManager::autoAddToSelection() const { return m_autoAddToSelection; }

void PickManager::setAutoAddToSelection(bool enabled) {
    if(m_autoAddToSelection == enabled) {
        return;
    }
    m_autoAddToSelection = enabled;
    emit autoAddToSelectionChanged();
}

bool PickManager::singlePickActive() const { return m_singlePickActive; }

void PickManager::clearSelection() {
    setSelectedEntitiesInternal({});
    saveSelectionForContext(effectiveContextKey());
    emit selectionChanged(effectiveContextKey(), QVariant::fromValue(m_selectedEntities));
}

void PickManager::addSelection(const QString& entityType, int entityUid) {
    for(const auto& v : m_selectedEntities) {
        const auto map = v.toMap();
        if(map.value("type").toString() == entityType && map.value("uid").toInt() == entityUid) {
            return;
        }
    }

    QVariantMap entry;
    entry.insert("type", entityType);
    entry.insert("uid", entityUid);

    auto newEntities = m_selectedEntities;
    newEntities.push_back(entry);

    setSelectedEntitiesInternal(newEntities);
    saveSelectionForContext(effectiveContextKey());
    emit selectionChanged(effectiveContextKey(), QVariant::fromValue(m_selectedEntities));
}

void PickManager::removeSelection(const QString& entityType, int entityUid) {
    QVariantList newEntities;
    newEntities.reserve(m_selectedEntities.size());

    for(const auto& v : m_selectedEntities) {
        const auto map = v.toMap();
        const bool same =
            (map.value("type").toString() == entityType && map.value("uid").toInt() == entityUid);
        if(!same) {
            newEntities.push_back(map);
        }
    }

    setSelectedEntitiesInternal(newEntities);
    saveSelectionForContext(effectiveContextKey());
    emit selectionChanged(effectiveContextKey(), QVariant::fromValue(m_selectedEntities));
}

void PickManager::activatePickMode(const QString& entityType) {
    if(m_selectedType == entityType && m_pickModeActive) {
        setPickModeInternal(false, {});
    } else {
        setPickModeInternal(true, entityType);
    }

    emit pickModeChanged(effectiveContextKey(), m_pickModeActive, m_selectedType);
}

void PickManager::deactivatePickMode() {
    setPickModeInternal(false, {});
    emit pickModeChanged(effectiveContextKey(), false, {});
}

void PickManager::requestSinglePick(const QString& entityType, const QString& consumerKey) {
    if(!consumerKey.isEmpty()) {
        setActiveConsumer(consumerKey);
    }

    m_singlePickActive = true;
    emit singlePickActiveChanged();

    m_restoreAutoAddAfterSinglePick = true;
    setAutoAddToSelection(false);

    setPickModeInternal(true, entityType);

    const QString ctx = effectiveContextKey();
    emit pickModeChanged(ctx, true, m_selectedType);
    emit singlePickStarted(ctx, m_selectedType);
}

void PickManager::onViewportEntityPicked(const QString& entityType, int entityUid) {
    if(!m_pickModeActive) {
        return;
    }

    const QString ctx = effectiveContextKey();
    emit entityPicked(ctx, entityType, entityUid);

    if(m_singlePickActive) {
        m_singlePickActive = false;
        emit singlePickActiveChanged();

        emit singlePickFinished(ctx, entityType, entityUid);

        if(m_restoreAutoAddAfterSinglePick) {
            setAutoAddToSelection(true);
        }

        deactivatePickMode();
        return;
    }

    if(m_autoAddToSelection) {
        addSelection(entityType, entityUid);
    }
}

void PickManager::onViewportEntityUnpicked(const QString& entityType, int entityUid) {
    // Right-click unpick: remove entity from selection if it exists
    if(!m_pickModeActive) {
        return;
    }

    // Check if entity is in selection
    bool found = false;
    for(const auto& v : m_selectedEntities) {
        const auto map = v.toMap();
        if(map.value("type").toString() == entityType && map.value("uid").toInt() == entityUid) {
            found = true;
            break;
        }
    }

    if(found) {
        // Entity is selected, remove it
        removeSelection(entityType, entityUid);
    }
    // If not found, do nothing (right-click on unselected entity has no effect)
}

void PickManager::onViewportPickCancelled() {
    const QString ctx = effectiveContextKey();

    if(m_singlePickActive) {
        m_singlePickActive = false;
        emit singlePickActiveChanged();

        emit singlePickCancelled(ctx);

        if(m_restoreAutoAddAfterSinglePick) {
            setAutoAddToSelection(true);
        }
    }

    if(m_pickModeActive) {
        deactivatePickMode();
    }

    emit pickCancelled(ctx);
}

QString PickManager::effectiveContextKey() const {
    return !m_activeConsumerKey.isEmpty() ? m_activeConsumerKey : m_contextKey;
}

void PickManager::ensureContext(const QString& key) {
    if(key.isEmpty()) {
        return;
    }
    if(!m_selectionByContext.contains(key)) {
        m_selectionByContext.insert(key, {});
    }
}

void PickManager::loadSelectionForContext(const QString& key) {
    if(key.isEmpty()) {
        setSelectedEntitiesInternal({});
        return;
    }

    ensureContext(key);
    setSelectedEntitiesInternal(m_selectionByContext.value(key));
}

void PickManager::saveSelectionForContext(const QString& key) {
    if(key.isEmpty()) {
        return;
    }
    m_selectionByContext.insert(key, m_selectedEntities);
}

void PickManager::applyViewportState() {
    if(!m_viewport) {
        return;
    }

    m_viewport->setProperty("pickModeEnabled", m_pickModeActive);
    if(!m_selectedType.isEmpty()) {
        m_viewport->setProperty(
            "pickEntityType",
            QVariant::fromValue(Geometry::entityTypeFromString(m_selectedType.toStdString())));
    }
}

void PickManager::setPickModeInternal(bool enabled, const QString& entityType) {
    const bool modeChanged = (m_pickModeActive != enabled);
    const bool typeChanged = (m_selectedType != entityType);

    m_pickModeActive = enabled;
    m_selectedType = enabled ? entityType : QString{};

    if(modeChanged) {
        emit pickModeActiveChanged();
    }
    if(typeChanged) {
        emit selectedTypeChanged();
    }

    applyViewportState();
}

void PickManager::setSelectedEntitiesInternal(const QVariantList& entities) {
    if(m_selectedEntities == entities) {
        return;
    }
    m_selectedEntities = entities;

    // Sync selected UIDs to viewport for hover highlight exclusion
    syncSelectedUidsToViewport();

    emit selectedEntitiesChanged();
}

void PickManager::syncSelectedUidsToViewport() {
    if(!m_viewport) {
        return;
    }

    // Extract UIDs from selected entities and set to viewport
    // Cast to GLViewport to call the method directly
    auto* glViewport = qobject_cast<OpenGeoLab::App::GLViewport*>(m_viewport.data());
    if(glViewport) {
        std::vector<Geometry::EntityUID> uids;
        uids.reserve(m_selectedEntities.size());
        for(const auto& v : m_selectedEntities) {
            const auto map = v.toMap();
            uids.push_back(static_cast<Geometry::EntityUID>(map.value("uid").toInt()));
        }
        glViewport->setSelectedEntityUids(uids);
    }
}

} // namespace OpenGeoLab::App
