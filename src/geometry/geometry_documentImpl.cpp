/**
 * @file geometry_document.cpp
 * @brief Implementation of GeometryDocument entity management
 */

#include "geometry_documentImpl.hpp"
#include "entity/geometry_entity.hpp"
#include "shape_builder.hpp"
#include "util/logger.hpp"
#include "util/progress_callback.hpp"

#include <queue>
#include <unordered_set>

namespace OpenGeoLab::Geometry {

bool GeometryDocumentImpl::addEntity(const GeometryEntityPtr& entity) {
    if(!m_entityIndex.addEntity(entity)) {
        return false;
    }
    entity->setDocument(shared_from_this());
    return true;
}

bool GeometryDocumentImpl::removeEntity(EntityId entity_id) {
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

size_t GeometryDocumentImpl::removeEntityWithChildren(EntityId entity_id) {
    size_t removed_count = 0;
    removeEntityRecursive(entity_id, removed_count);
    return removed_count;
}

void GeometryDocumentImpl::removeEntityRecursive(EntityId entity_id, // NOLINT
                                                 size_t& removed_count) {
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

void GeometryDocumentImpl::clear() { m_entityIndex.clear(); }

GeometryEntityPtr GeometryDocumentImpl::findById(EntityId entity_id) const {
    return m_entityIndex.findById(entity_id);
}

GeometryEntityPtr GeometryDocumentImpl::findByUIDAndType(EntityUID entity_uid,
                                                         EntityType entity_type) const {
    return m_entityIndex.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityPtr GeometryDocumentImpl::findByShape(const TopoDS_Shape& shape) const {
    return m_entityIndex.findByShape(shape);
}

[[nodiscard]] size_t GeometryDocumentImpl::entityCount() const {
    return m_entityIndex.entityCount();
}

[[nodiscard]] size_t GeometryDocumentImpl::entityCountByType(EntityType entity_type) const {
    return m_entityIndex.entityCountByType(entity_type);
}

std::vector<GeometryEntityPtr> GeometryDocumentImpl::entitiesByType(EntityType entity_type) const {
    return m_entityIndex.entitiesByType(entity_type);
}

std::vector<GeometryEntityPtr> GeometryDocumentImpl::allEntities() const {
    return m_entityIndex.snapshotEntities();
}

std::vector<GeometryEntityPtr> GeometryDocumentImpl::findAncestors(EntityId entity_id,
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

std::vector<GeometryEntityPtr>
GeometryDocumentImpl::findDescendants(EntityId entity_id, EntityType descendant_type) const {
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

GeometryEntityPtr GeometryDocumentImpl::findOwningPart(EntityId entity_id) const {
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
GeometryDocumentImpl::findRelatedEntities(EntityId edge_entity_id, EntityType related_type) const {
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

bool GeometryDocumentImpl::addChildEdge(EntityId parent_id, EntityId child_id) {
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

bool GeometryDocumentImpl::removeChildEdge(EntityId parent_id, EntityId child_id) {
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

LoadResult GeometryDocumentImpl::loadFromShape(const TopoDS_Shape& shape,
                                               const std::string& name,
                                               Util::ProgressCallback progress) {
    if(shape.IsNull()) {
        return LoadResult::failure("Input shape is null");
    }

    if(!progress(0.0, "Starting shape load...")) {
        return LoadResult::failure("Operation cancelled");
    }

    try {
        ShapeBuilder builder(shared_from_this());

        auto subcallback = Util::makeScaledProgressCallback(progress, 0.0, 0.9);

        auto build_result = builder.buildFromShape(shape, name, subcallback);

        if(!build_result.m_success) {
            return LoadResult::failure(build_result.m_errorMessage);
        }

        if(!progress(0.95, "Finalizing...")) {
            return LoadResult::failure("Operation cancelled");
        }

        progress(1.0, "Load completed.");
        return LoadResult::success(build_result.m_rootPart->entityId(),
                                   build_result.totalEntityCount());
    } catch(const std::exception& e) {
        LOG_ERROR("Exception during shape load: {}", e.what());
        return LoadResult::failure(std::string("Exception: ") + e.what());
    }
}

} // namespace OpenGeoLab::Geometry
