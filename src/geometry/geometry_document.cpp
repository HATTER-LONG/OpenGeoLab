/**
 * @file geometry_document.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry/geometry_document.hpp"

namespace OpenGeoLab::Geometry {

bool GeometryDocument::addEntity(const GeometryEntityPtr& entity) {
    if(!m_entityIndex.addEntity(entity)) {
        return false;
    }
    entity->setDocument(shared_from_this());
    return true;
}

bool GeometryDocument::removeEntity(EntityId entity_id) {
    const auto entity = m_entityIndex.findById(entity_id);
    if(!entity) {
        return false;
    }

    if(!m_entityIndex.removeEntity(entity_id)) {
        return false;
    }

    entity->setDocument({});
    return true;
}

void GeometryDocument::clear() { m_entityIndex.clear(); }

GeometryEntityPtr GeometryDocument::findById(EntityId entity_id) const {
    return m_entityIndex.findById(entity_id);
}

GeometryEntityPtr GeometryDocument::findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const {
    return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityPtr GeometryDocument::findByShape(const TopoDS_Shape& shape) const {
    return m_entityIndex.findByShape(shape);
}

[[nodiscard]] size_t GeometryDocument::entityCount() const { return m_entityIndex.entityCount(); }

[[nodiscard]] size_t GeometryDocument::entityCountByType(EntityType entity_type) const {
    return m_entityIndex.entityCountByType(entity_type);
}
bool GeometryDocument::addChildEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    const auto parent = findById(parent_id);
    const auto child = findById(child_id);
    if(!parent || !child) {
        return false;
    }

    // Enforce type-level relationship constraints.
    if(!parent->canAddChildType(child->entityType()) ||
       !child->canAddParentType(parent->entityType())) {
        return false;
    }

    const bool inserted = parent->addChildNoSync(child_id);
    if(!inserted) {
        return false;
    }

    (void)child->addParentNoSync(parent_id);
    return true;
}

bool GeometryDocument::removeChildEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    const auto parent = findById(parent_id);
    if(!parent) {
        return false;
    }

    const bool erased = parent->removeChildNoSync(child_id);
    if(!erased) {
        return false;
    }

    // Best-effort: if child already expired, local removal is enough.
    if(const auto child = findById(child_id)) {
        (void)child->removeParentNoSync(parent_id);
    }

    return true;
}

GeometryDocumentManager& GeometryDocumentManager::instance() {
    static GeometryDocumentManager s_instance;
    return s_instance;
}

GeometryDocumentPtr GeometryDocumentManager::currentDocument() {
    if(!m_currentDocument) {
        return newDocument();
    }
    return m_currentDocument;
}

GeometryDocumentPtr GeometryDocumentManager::newDocument() {
    m_currentDocument = GeometryDocument::create();
    return m_currentDocument;
}
} // namespace OpenGeoLab::Geometry
