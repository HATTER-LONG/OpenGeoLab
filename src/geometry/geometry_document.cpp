/**
 * @file geometry_document.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry/geometry_document.hpp"

#include <queue>
#include <unordered_set>

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

size_t GeometryDocument::removeEntityWithChildren(EntityId entity_id) {
    size_t removed_count = 0;
    removeEntityRecursive(entity_id, removed_count);
    return removed_count;
}

void GeometryDocument::removeEntityRecursive(EntityId entity_id, size_t& removed_count) { // NOLINT
    const auto entity = m_entityIndex.findById(entity_id);
    if(!entity) {
        return;
    }

    // First, recursively remove all children
    auto children = entity->children();
    for(const auto& child : children) {
        if(child) {
            removeEntityRecursive(child->entityId(), removed_count);
        }
    }

    // Then remove this entity
    if(removeEntity(entity_id)) {
        ++removed_count;
    }
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

std::vector<GeometryEntityPtr> GeometryDocument::entitiesByType(EntityType entity_type) const {
    return m_entityIndex.entitiesByType(entity_type);
}

std::vector<GeometryEntityPtr> GeometryDocument::allEntities() const {
    return m_entityIndex.snapshotEntities();
}

std::vector<GeometryEntityPtr> GeometryDocument::findAncestors(EntityId entity_id,
                                                               EntityType ancestor_type) const {
    std::vector<GeometryEntityPtr> result;
    const auto entity = findById(entity_id);
    if(!entity) {
        return result;
    }

    // BFS traversal up the parent chain
    std::unordered_set<EntityId> visited;
    std::queue<EntityId> to_visit;

    for(const auto& parent : entity->parents()) {
        if(parent) {
            to_visit.push(parent->entityId());
        }
    }

    while(!to_visit.empty()) {
        EntityId current_id = to_visit.front();
        to_visit.pop();

        if(visited.count(current_id) > 0) {
            continue;
        }
        visited.insert(current_id);

        auto current = findById(current_id);
        if(!current) {
            continue;
        }

        if(current->entityType() == ancestor_type) {
            result.push_back(current);
        }

        // Continue searching through parents
        for(const auto& parent : current->parents()) {
            if(parent && visited.count(parent->entityId()) == 0) {
                to_visit.push(parent->entityId());
            }
        }
    }

    return result;
}

std::vector<GeometryEntityPtr> GeometryDocument::findDescendants(EntityId entity_id,
                                                                 EntityType descendant_type) const {
    std::vector<GeometryEntityPtr> result;
    const auto entity = findById(entity_id);
    if(!entity) {
        return result;
    }

    // BFS traversal down the child tree
    std::unordered_set<EntityId> visited;
    std::queue<EntityId> to_visit;

    for(const auto& child : entity->children()) {
        if(child) {
            to_visit.push(child->entityId());
        }
    }

    while(!to_visit.empty()) {
        EntityId current_id = to_visit.front();
        to_visit.pop();

        if(visited.count(current_id) > 0) {
            continue;
        }
        visited.insert(current_id);

        auto current = findById(current_id);
        if(!current) {
            continue;
        }

        if(current->entityType() == descendant_type) {
            result.push_back(current);
        }

        // Continue searching through children
        for(const auto& child : current->children()) {
            if(child && visited.count(child->entityId()) == 0) {
                to_visit.push(child->entityId());
            }
        }
    }

    return result;
}

GeometryEntityPtr GeometryDocument::findOwningPart(EntityId entity_id) const {
    const auto entity = findById(entity_id);
    if(!entity) {
        return nullptr;
    }

    // If this is already a Part, return it
    if(entity->entityType() == EntityType::Part) {
        return entity;
    }

    // Traverse up the parent chain to find a Part
    std::unordered_set<EntityId> visited;
    std::queue<EntityId> to_visit;

    for(const auto& parent : entity->parents()) {
        if(parent) {
            to_visit.push(parent->entityId());
        }
    }

    while(!to_visit.empty()) {
        EntityId current_id = to_visit.front();
        to_visit.pop();

        if(visited.count(current_id) > 0) {
            continue;
        }
        visited.insert(current_id);

        auto current = findById(current_id);
        if(!current) {
            continue;
        }

        if(current->entityType() == EntityType::Part) {
            return current;
        }

        for(const auto& parent : current->parents()) {
            if(parent && visited.count(parent->entityId()) == 0) {
                to_visit.push(parent->entityId());
            }
        }
    }

    return nullptr;
}

std::vector<GeometryEntityPtr>
GeometryDocument::findRelatedEntities(EntityId edge_entity_id, EntityType related_type) const {
    std::vector<GeometryEntityPtr> result;

    const auto entity = findById(edge_entity_id);
    if(!entity) {
        return result;
    }

    // For edges, traverse up to find parent wires, then to faces
    if(entity->entityType() == EntityType::Edge && related_type == EntityType::Face) {
        // Edge -> Wire -> Face
        for(const auto& wire : entity->parents()) {
            if(!wire || wire->entityType() != EntityType::Wire) {
                continue;
            }
            for(const auto& face : wire->parents()) {
                if(face && face->entityType() == EntityType::Face) {
                    // Check for duplicates
                    bool found = false;
                    for(const auto& existing : result) {
                        if(existing->entityId() == face->entityId()) {
                            found = true;
                            break;
                        }
                    }
                    if(!found) {
                        result.push_back(face);
                    }
                }
            }
        }
    } else {
        // Generic approach: use findAncestors for other relationships
        result = findAncestors(edge_entity_id, related_type);
    }

    return result;
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
