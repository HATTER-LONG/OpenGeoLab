/**
 * @file relationship_index.cpp
 * @brief Implementation of relationship index
 */

#include "relationship_index.hpp"

#include <queue>

namespace OpenGeoLab::Geometry {

void RelationshipIndex::clear() {
    std::unique_lock lock(m_indexMutex);
    m_typeById.clear();
    m_childrenByParent.clear();
    m_parentsByChild.clear();
    m_relatedById.clear();
}

void RelationshipIndex::addEntity(EntityId entity_id, EntityType entity_type) {
    if(entity_id == INVALID_ENTITY_ID) {
        return;
    }

    std::unique_lock lock(m_indexMutex);
    m_typeById[entity_id] = entity_type;
    m_relatedById.emplace(entity_id, RelatedTargets{});
}

void RelationshipIndex::removeEntity(EntityId entity_id) {
    if(entity_id == INVALID_ENTITY_ID) {
        return;
    }

    std::unique_lock lock(m_indexMutex);

    auto parents_it = m_parentsByChild.find(entity_id);
    if(parents_it != m_parentsByChild.end()) {
        for(const EntityId parent_id : parents_it->second) {
            auto child_it = m_childrenByParent.find(parent_id);
            if(child_it != m_childrenByParent.end()) {
                child_it->second.erase(entity_id);
                if(child_it->second.empty()) {
                    m_childrenByParent.erase(child_it);
                }
            }
        }
        m_parentsByChild.erase(parents_it);
    }

    auto children_it = m_childrenByParent.find(entity_id);
    if(children_it != m_childrenByParent.end()) {
        for(const EntityId child_id : children_it->second) {
            auto parent_it = m_parentsByChild.find(child_id);
            if(parent_it != m_parentsByChild.end()) {
                parent_it->second.erase(entity_id);
                if(parent_it->second.empty()) {
                    m_parentsByChild.erase(parent_it);
                }
            }
        }
        m_childrenByParent.erase(children_it);
    }

    m_typeById.erase(entity_id);
    m_relatedById.erase(entity_id);

    rebuildRelatedMappingsLocked();
}

bool RelationshipIndex::addEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    std::unique_lock lock(m_indexMutex);

    if(m_typeById.find(parent_id) == m_typeById.end() ||
       m_typeById.find(child_id) == m_typeById.end()) {
        return false;
    }

    auto& children = m_childrenByParent[parent_id];
    if(!children.insert(child_id).second) {
        return false;
    }

    m_parentsByChild[child_id].insert(parent_id);

    auto ancestors = collectAncestorsLocked(parent_id);
    ancestors.insert(parent_id);

    auto descendants = collectDescendantsLocked(child_id);
    descendants.insert(child_id);

    for(const EntityId ancestor_id : ancestors) {
        for(const EntityId descendant_id : descendants) {
            if(ancestor_id == descendant_id) {
                continue;
            }

            addRelationLocked(descendant_id, ancestor_id);
            addRelationLocked(ancestor_id, descendant_id);
        }
    }

    return true;
}

bool RelationshipIndex::removeEdge(EntityId parent_id, EntityId child_id) {
    if(parent_id == INVALID_ENTITY_ID || child_id == INVALID_ENTITY_ID) {
        return false;
    }
    if(parent_id == child_id) {
        return false;
    }

    std::unique_lock lock(m_indexMutex);

    auto child_it = m_childrenByParent.find(parent_id);
    if(child_it == m_childrenByParent.end()) {
        return false;
    }

    if(child_it->second.erase(child_id) == 0) {
        return false;
    }

    if(child_it->second.empty()) {
        m_childrenByParent.erase(child_it);
    }

    auto parent_it = m_parentsByChild.find(child_id);
    if(parent_it != m_parentsByChild.end()) {
        parent_it->second.erase(parent_id);
        if(parent_it->second.empty()) {
            m_parentsByChild.erase(parent_it);
        }
    }

    rebuildRelatedMappingsLocked();
    return true;
}

void RelationshipIndex::detachAllRelations(EntityId entity_id) {
    if(entity_id == INVALID_ENTITY_ID) {
        return;
    }

    std::unique_lock lock(m_indexMutex);

    auto parents_it = m_parentsByChild.find(entity_id);
    if(parents_it != m_parentsByChild.end()) {
        for(const EntityId parent_id : parents_it->second) {
            auto child_it = m_childrenByParent.find(parent_id);
            if(child_it != m_childrenByParent.end()) {
                child_it->second.erase(entity_id);
                if(child_it->second.empty()) {
                    m_childrenByParent.erase(child_it);
                }
            }
        }
        m_parentsByChild.erase(parents_it);
    }

    auto children_it = m_childrenByParent.find(entity_id);
    if(children_it != m_childrenByParent.end()) {
        for(const EntityId child_id : children_it->second) {
            auto parent_it = m_parentsByChild.find(child_id);
            if(parent_it != m_parentsByChild.end()) {
                parent_it->second.erase(entity_id);
                if(parent_it->second.empty()) {
                    m_parentsByChild.erase(parent_it);
                }
            }
        }
        m_childrenByParent.erase(children_it);
    }

    rebuildRelatedMappingsLocked();
}

std::vector<EntityId> RelationshipIndex::parentIds(EntityId entity_id) const {
    std::shared_lock lock(m_indexMutex);
    std::vector<EntityId> result;
    const auto it = m_parentsByChild.find(entity_id);
    if(it == m_parentsByChild.end()) {
        return result;
    }
    appendSnapshot(it->second, result);
    return result;
}

std::vector<EntityId> RelationshipIndex::childIds(EntityId entity_id) const {
    std::shared_lock lock(m_indexMutex);
    std::vector<EntityId> result;
    const auto it = m_childrenByParent.find(entity_id);
    if(it == m_childrenByParent.end()) {
        return result;
    }
    appendSnapshot(it->second, result);
    return result;
}

bool RelationshipIndex::hasParent(EntityId child_id, EntityId parent_id) const {
    std::shared_lock lock(m_indexMutex);
    const auto it = m_parentsByChild.find(child_id);
    if(it == m_parentsByChild.end()) {
        return false;
    }
    return it->second.count(parent_id) > 0;
}

bool RelationshipIndex::hasChild(EntityId parent_id, EntityId child_id) const {
    std::shared_lock lock(m_indexMutex);
    const auto it = m_childrenByParent.find(parent_id);
    if(it == m_childrenByParent.end()) {
        return false;
    }
    return it->second.count(child_id) > 0;
}

size_t RelationshipIndex::parentCount(EntityId entity_id) const {
    std::shared_lock lock(m_indexMutex);
    const auto it = m_parentsByChild.find(entity_id);
    if(it == m_parentsByChild.end()) {
        return 0;
    }
    return it->second.size();
}

size_t RelationshipIndex::childCount(EntityId entity_id) const {
    std::shared_lock lock(m_indexMutex);
    const auto it = m_childrenByParent.find(entity_id);
    if(it == m_childrenByParent.end()) {
        return 0;
    }
    return it->second.size();
}

std::vector<EntityId> RelationshipIndex::findRelated(EntityId source_id,
                                                     EntityType target_type) const {
    std::shared_lock lock(m_indexMutex);
    std::vector<EntityId> result;
    const auto it = m_relatedById.find(source_id);
    if(it == m_relatedById.end()) {
        return result;
    }

    const RelatedTargets& targets = it->second;
    switch(target_type) {
    case EntityType::Vertex:
        appendSnapshot(targets.m_nodes, result);
        break;
    case EntityType::Edge:
        appendSnapshot(targets.m_edges, result);
        break;
    case EntityType::Wire:
        appendSnapshot(targets.m_wires, result);
        break;
    case EntityType::Face:
        appendSnapshot(targets.m_faces, result);
        break;
    case EntityType::Solid:
        appendSnapshot(targets.m_solids, result);
        break;
    case EntityType::Part:
        appendSnapshot(targets.m_parts, result);
        break;
    default:
        break;
    }

    return result;
}

PartMembers RelationshipIndex::getPartMembers(EntityId part_id) const {
    std::shared_lock lock(m_indexMutex);
    PartMembers members;
    const auto it = m_relatedById.find(part_id);
    if(it == m_relatedById.end()) {
        return members;
    }

    appendSnapshot(it->second.m_nodes, members.m_nodes);
    appendSnapshot(it->second.m_edges, members.m_edges);
    appendSnapshot(it->second.m_wires, members.m_wires);
    appendSnapshot(it->second.m_faces, members.m_faces);
    appendSnapshot(it->second.m_solids, members.m_solids);

    return members;
}

bool RelationshipIndex::isIndexedType(EntityType type) {
    switch(type) {
    case EntityType::Vertex:
    case EntityType::Edge:
    case EntityType::Wire:
    case EntityType::Face:
    case EntityType::Solid:
    case EntityType::Part:
        return true;
    default:
        return false;
    }
}

void RelationshipIndex::rebuildRelatedMappingsLocked() {
    m_relatedById.clear();

    for(const auto& [entity_id, entity_type] : m_typeById) {
        (void)entity_type;
        m_relatedById.emplace(entity_id, RelatedTargets{});
    }

    for(const auto& [entity_id, entity_type] : m_typeById) {
        (void)entity_type;
        auto ancestors = collectAncestorsLocked(entity_id);
        auto descendants = collectDescendantsLocked(entity_id);

        for(const EntityId ancestor_id : ancestors) {
            if(ancestor_id == entity_id) {
                continue;
            }
            addRelationLocked(entity_id, ancestor_id);
        }

        for(const EntityId descendant_id : descendants) {
            if(descendant_id == entity_id) {
                continue;
            }
            addRelationLocked(entity_id, descendant_id);
        }
    }
}

std::unordered_set<EntityId> RelationshipIndex::collectAncestorsLocked(EntityId entity_id) const {
    std::unordered_set<EntityId> result;
    std::queue<EntityId> to_visit;

    const auto it = m_parentsByChild.find(entity_id);
    if(it != m_parentsByChild.end()) {
        for(const EntityId parent_id : it->second) {
            to_visit.push(parent_id);
        }
    }

    while(!to_visit.empty()) {
        const EntityId current_id = to_visit.front();
        to_visit.pop();

        if(!result.insert(current_id).second) {
            continue;
        }

        const auto parent_it = m_parentsByChild.find(current_id);
        if(parent_it != m_parentsByChild.end()) {
            for(const EntityId parent_id : parent_it->second) {
                if(result.count(parent_id) == 0) {
                    to_visit.push(parent_id);
                }
            }
        }
    }

    return result;
}

std::unordered_set<EntityId> RelationshipIndex::collectDescendantsLocked(EntityId entity_id) const {
    std::unordered_set<EntityId> result;
    std::queue<EntityId> to_visit;

    const auto it = m_childrenByParent.find(entity_id);
    if(it != m_childrenByParent.end()) {
        for(const EntityId child_id : it->second) {
            to_visit.push(child_id);
        }
    }

    while(!to_visit.empty()) {
        const EntityId current_id = to_visit.front();
        to_visit.pop();

        if(!result.insert(current_id).second) {
            continue;
        }

        const auto child_it = m_childrenByParent.find(current_id);
        if(child_it != m_childrenByParent.end()) {
            for(const EntityId child_id : child_it->second) {
                if(result.count(child_id) == 0) {
                    to_visit.push(child_id);
                }
            }
        }
    }

    return result;
}

void RelationshipIndex::addRelationLocked(EntityId source_id, EntityId target_id) {
    const auto type_it = m_typeById.find(target_id);
    if(type_it == m_typeById.end()) {
        return;
    }

    const EntityType target_type = type_it->second;
    if(!isIndexedType(target_type)) {
        return;
    }

    auto& targets = m_relatedById[source_id];
    switch(target_type) {
    case EntityType::Vertex:
        targets.m_nodes.insert(target_id);
        break;
    case EntityType::Edge:
        targets.m_edges.insert(target_id);
        break;
    case EntityType::Wire:
        targets.m_wires.insert(target_id);
        break;
    case EntityType::Face:
        targets.m_faces.insert(target_id);
        break;
    case EntityType::Solid:
        targets.m_solids.insert(target_id);
        break;
    case EntityType::Part:
        targets.m_parts.insert(target_id);
        break;
    default:
        break;
    }
}

void RelationshipIndex::appendSnapshot(const std::unordered_set<EntityId>& source,
                                       std::vector<EntityId>& output) {
    output.reserve(output.size() + source.size());
    for(const EntityId id : source) {
        output.push_back(id);
    }
}

} // namespace OpenGeoLab::Geometry
