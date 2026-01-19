/**
 * @file geometry_document.cpp
 * @brief Implementation of GeometryDocument
 */

#include "geometry/geometry_document.hpp"
#include "util/logger.hpp"

#include <algorithm>

namespace OpenGeoLab::Geometry {

GeometryDocument::GeometryDocument() = default;

GeometryDocumentPtr GeometryDocument::instance() {
    static auto instance = std::make_shared<GeometryDocument>();
    return instance;
}

// ============================================================================
// Part Management
// ============================================================================

bool GeometryDocument::addPart(std::shared_ptr<PartEntity> part) {
    if(!part) {
        LOG_WARN("Attempted to add null part to document");
        return false;
    }

    // Check for duplicate
    for(const auto& existing : m_parts) {
        if(existing->id() == part->id()) {
            LOG_WARN("Part with ID {} already exists in document", part->id());
            return false;
        }
    }

    m_parts.push_back(part);
    updateEntityIndex(part);
    ++m_version;
    notifyPartAdded(part);

    LOG_INFO("Added part '{}' (ID: {}) to document", part->name(), part->id());
    return true;
}

bool GeometryDocument::removePart(EntityId partId) {
    auto iter = std::find_if(m_parts.begin(), m_parts.end(),
                             [partId](const auto& part) { return part->id() == partId; });

    if(iter == m_parts.end()) {
        return false;
    }

    // Remove from selection if selected
    auto selIter = std::find(m_selectedIds.begin(), m_selectedIds.end(), partId);
    if(selIter != m_selectedIds.end()) {
        m_selectedIds.erase(selIter);
    }

    // Remove from entity index
    m_entityIndex.erase(partId);
    m_entityToPartMap.erase(partId);

    // Remove child entities from index
    std::function<void(const GeometryEntityPtr&)> removeFromIndex;
    removeFromIndex = [this, &removeFromIndex](const GeometryEntityPtr& entity) {
        m_entityIndex.erase(entity->id());
        m_entityToPartMap.erase(entity->id());
        for(const auto& child : entity->children()) {
            removeFromIndex(child);
        }
    };
    removeFromIndex(*iter);

    m_parts.erase(iter);
    ++m_version;
    notifyPartRemoved(partId);

    LOG_INFO("Removed part ID {} from document", partId);
    return true;
}

std::shared_ptr<PartEntity> GeometryDocument::findPart(EntityId partId) const {
    auto iter = std::find_if(m_parts.begin(), m_parts.end(),
                             [partId](const auto& part) { return part->id() == partId; });
    return (iter != m_parts.end()) ? *iter : nullptr;
}

void GeometryDocument::clear() {
    m_parts.clear();
    m_entityIndex.clear();
    m_entityToPartMap.clear();
    m_selectedIds.clear();
    ++m_version;
    notifyDocumentCleared();
    LOG_INFO("Document cleared");
}

// ============================================================================
// Entity Lookup
// ============================================================================

GeometryEntityPtr GeometryDocument::findEntity(EntityId entityId) const {
    auto iter = m_entityIndex.find(entityId);
    if(iter != m_entityIndex.end()) {
        return iter->second.lock();
    }
    return nullptr;
}

std::shared_ptr<PartEntity> GeometryDocument::findPartContaining(EntityId entityId) const {
    auto iter = m_entityToPartMap.find(entityId);
    if(iter != m_entityToPartMap.end()) {
        return findPart(iter->second);
    }
    return nullptr;
}

// ============================================================================
// Selection Management
// ============================================================================

void GeometryDocument::setSelectionMode(SelectionMode mode) {
    if(m_selectionMode == mode) {
        return;
    }
    m_selectionMode = mode;
    clearSelection();
    LOG_DEBUG("Selection mode changed to {}", static_cast<int>(mode));
}

void GeometryDocument::select(EntityId entityId, bool addToSelection) {
    if(!addToSelection) {
        clearSelection();
    }

    if(isSelected(entityId)) {
        return;
    }

    auto entity = findEntity(entityId);
    if(!entity) {
        LOG_WARN("Cannot select entity {}: not found", entityId);
        return;
    }

    entity->setSelected(true);
    m_selectedIds.push_back(entityId);
    notifySelectionChanged();

    LOG_DEBUG("Selected entity {} (total: {})", entityId, m_selectedIds.size());
}

void GeometryDocument::deselect(EntityId entityId) {
    auto iter = std::find(m_selectedIds.begin(), m_selectedIds.end(), entityId);
    if(iter == m_selectedIds.end()) {
        return;
    }

    if(auto entity = findEntity(entityId)) {
        entity->setSelected(false);
    }

    m_selectedIds.erase(iter);
    notifySelectionChanged();

    LOG_DEBUG("Deselected entity {}", entityId);
}

void GeometryDocument::clearSelection() {
    if(m_selectedIds.empty()) {
        return; // Nothing to clear, no notification needed
    }

    for(EntityId id : m_selectedIds) {
        if(auto entity = findEntity(id)) {
            entity->setSelected(false);
        }
    }
    m_selectedIds.clear();
    notifySelectionChanged();
}

bool GeometryDocument::isSelected(EntityId entityId) const {
    return std::find(m_selectedIds.begin(), m_selectedIds.end(), entityId) != m_selectedIds.end();
}

GeometryEntityPtr GeometryDocument::primarySelection() const {
    if(m_selectedIds.empty()) {
        return nullptr;
    }
    return findEntity(m_selectedIds.front());
}

// ============================================================================
// Visibility Management
// ============================================================================

void GeometryDocument::setEntityVisible(EntityId entityId, bool visible) {
    auto entity = findEntity(entityId);
    if(!entity) {
        return;
    }

    if(entity->isVisible() == visible) {
        return;
    }

    entity->setVisible(visible);
    notifyVisibilityChanged(entityId, visible);
}

void GeometryDocument::showAll() {
    for(const auto& part : m_parts) {
        part->setVisible(true);
        notifyVisibilityChanged(part->id(), true);
    }
}

void GeometryDocument::hideAll() {
    for(const auto& part : m_parts) {
        part->setVisible(false);
        notifyVisibilityChanged(part->id(), false);
    }
}

// ============================================================================
// Bounding Box
// ============================================================================

BoundingBox GeometryDocument::totalBoundingBox() const {
    BoundingBox result;
    bool first = true;

    for(const auto& part : m_parts) {
        if(!part->isVisible()) {
            continue;
        }

        BoundingBox partBox = part->boundingBox();
        if(!partBox.isValid()) {
            continue;
        }

        if(first) {
            result = partBox;
            first = false;
        } else {
            result.expand(partBox);
        }
    }

    return result;
}

// ============================================================================
// Observer Pattern
// ============================================================================

void GeometryDocument::addObserver(IDocumentObserverPtr observer) {
    if(!observer) {
        return;
    }
    m_observers.push_back(observer);
}

void GeometryDocument::removeObserver(IDocumentObserverPtr observer) {
    auto iter = std::find(m_observers.begin(), m_observers.end(), observer);
    if(iter != m_observers.end()) {
        m_observers.erase(iter);
    }
}

void GeometryDocument::notifyPartAdded(const std::shared_ptr<PartEntity>& part) {
    for(const auto& observer : m_observers) {
        observer->onPartAdded(part);
    }
}

void GeometryDocument::notifyPartRemoved(EntityId partId) {
    for(const auto& observer : m_observers) {
        observer->onPartRemoved(partId);
    }
}

void GeometryDocument::notifySelectionChanged() {
    for(const auto& observer : m_observers) {
        observer->onSelectionChanged(m_selectedIds);
    }
}

void GeometryDocument::notifyVisibilityChanged(EntityId entityId, bool visible) {
    for(const auto& observer : m_observers) {
        observer->onVisibilityChanged(entityId, visible);
    }
}

void GeometryDocument::notifyDocumentCleared() {
    for(const auto& observer : m_observers) {
        observer->onDocumentCleared();
    }
}

void GeometryDocument::updateEntityIndex(const std::shared_ptr<PartEntity>& part) {
    std::function<void(const GeometryEntityPtr&, EntityId)> indexEntity;
    indexEntity = [this, &indexEntity](const GeometryEntityPtr& entity, EntityId partId) {
        m_entityIndex[entity->id()] = entity;
        m_entityToPartMap[entity->id()] = partId;
        for(const auto& child : entity->children()) {
            indexEntity(child, partId);
        }
    };

    indexEntity(part, part->id());
}

} // namespace OpenGeoLab::Geometry
