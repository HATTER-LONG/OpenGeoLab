#include "relationship_index.hpp"

#include "entity_index.hpp"

#include <queue>

namespace OpenGeoLab::Geometry {
EntityRelationshipIndex::EntityRelationshipIndex(EntityIndex& entity_index)
    : m_entityIndex(entity_index) {}

void EntityRelationshipIndex::invalidateCache() {
    m_reverseEntityToRelatedTargets.clear();
    m_fullDescendants.clear();
    m_cacheValid = false;
}

void EntityRelationshipIndex::clear() {
    std::unique_lock lock(m_indexMutex);
    m_directChildren.clear();
    m_directParents.clear();
    invalidateCache();
}

bool EntityRelationshipIndex::addRelationshipInfo(const GeometryEntityImpl& parent,
                                                  const GeometryEntityImpl& child) {
    const EntityKey parent_key = parent.entityKey();
    const EntityKey child_key = child.entityKey();
    if(!parent_key.isValid() || !child_key.isValid()) {
        return false;
    }
    if(parent_key == child_key) {
        return false;
    }

    std::unique_lock lock(m_indexMutex);

    (void)addChildNoCache(parent_key, child_key);
    invalidateCache();
    return true;
}

bool EntityRelationshipIndex::addChildNoCache(const EntityKey& parent, const EntityKey& child) {
    auto& children = m_directChildren[parent];
    const bool inserted = children.insert(child).second;

    auto& parents = m_directParents[child];
    (void)parents.insert(parent);

    return inserted;
}

void EntityRelationshipIndex::detachEntity(const GeometryEntityImpl& entity) {
    const EntityKey entity_key = entity.entityKey();
    if(!entity_key.isValid()) {
        return;
    }

    std::unique_lock lock(m_indexMutex);

    // Remove outgoing edges (entity_id -> children)
    auto out_it = m_directChildren.find(entity_key);
    if(out_it != m_directChildren.end()) {
        const auto children = out_it->second; // copy; we mutate maps below
        for(const auto& child_key : children) {
            auto parents_it = m_directParents.find(child_key);
            if(parents_it == m_directParents.end()) {
                continue;
            }
            (void)parents_it->second.erase(entity_key);
            if(parents_it->second.empty()) {
                m_directParents.erase(parents_it);
            }
        }
        m_directChildren.erase(out_it);
    }

    // Remove incoming edges (parents -> entity_id)
    auto in_it = m_directParents.find(entity_key);
    if(in_it != m_directParents.end()) {
        const auto parents = in_it->second; // copy; we mutate maps below
        for(const auto& parent_key : parents) {
            auto children_it = m_directChildren.find(parent_key);
            if(children_it == m_directChildren.end()) {
                continue;
            }
            (void)children_it->second.erase(entity_key);
            if(children_it->second.empty()) {
                m_directChildren.erase(children_it);
            }
        }
        m_directParents.erase(in_it);
    }

    invalidateCache();
}

bool EntityRelationshipIndex::buildRelationships() {
    std::unique_lock lock(m_indexMutex);

    m_reverseEntityToRelatedTargets.clear();
    m_fullDescendants.clear();

    const auto entities = m_entityIndex.snapshotEntities();

    // Build caches for fast ancestor/descendant queries.
    // - m_reverseEntityToRelatedTargets: walk upward (child -> parents)
    // - m_fullDescendants: walk downward (parent -> children)
    for(const auto& entity : entities) {
        if(!entity) {
            continue;
        }
        if(entity->entityType() == EntityType::None) {
            continue;
        }

        const EntityKey entity_key = entity->entityKey();

        {
            RelatedTargets targets;
            std::unordered_set<EntityKey, EntityKeyHash> visited;
            buildReverseRecursive(entity_key, targets, visited);
            m_reverseEntityToRelatedTargets[entity_key] = std::move(targets);
        }
        {
            RelatedTargets targets;
            std::unordered_set<EntityKey, EntityKeyHash> visited;
            buildDescendantsRecursive(entity_key, targets, visited);
            m_fullDescendants[entity_key] = std::move(targets);
        }
    }

    m_cacheValid = true;
    return true;
}

void EntityRelationshipIndex::buildReverseRecursive(
    const EntityKey& entity_key,
    RelatedTargets& targets,
    std::unordered_set<EntityKey, EntityKeyHash>& visited) {
    std::vector<EntityKey> stack;
    stack.push_back(entity_key);

    while(!stack.empty()) {
        const EntityKey current = stack.back();
        stack.pop_back();

        if(visited.count(current) > 0) {
            continue;
        }
        visited.insert(current);

        const auto it = m_directParents.find(current);
        if(it == m_directParents.end()) {
            continue;
        }

        for(const auto& parent_key : it->second) {
            switch(parent_key.m_type) {
            case EntityType::Part:
                targets.m_parts.insert(parent_key);
                break;
            case EntityType::Compound:
                targets.m_compounds.insert(parent_key);
                break;
            case EntityType::CompSolid:
                targets.m_compSolids.insert(parent_key);
                break;
            case EntityType::Solid:
                targets.m_solids.insert(parent_key);
                break;
            case EntityType::Shell:
                targets.m_shells.insert(parent_key);
                break;
            case EntityType::Face:
                targets.m_faces.insert(parent_key);
                break;
            case EntityType::Wire:
                targets.m_wires.insert(parent_key);
                break;
            case EntityType::Edge:
                targets.m_edges.insert(parent_key);
                break;
            case EntityType::Vertex:
                targets.m_nodes.insert(parent_key);
                break;
            default:
                break;
            }
            stack.push_back(parent_key);
        }
    }
}

void EntityRelationshipIndex::buildDescendantsRecursive(
    const EntityKey& entity_key,
    RelatedTargets& targets,
    std::unordered_set<EntityKey, EntityKeyHash>& visited) {
    std::vector<EntityKey> stack;
    stack.push_back(entity_key);

    while(!stack.empty()) {
        const EntityKey current = stack.back();
        stack.pop_back();

        if(visited.count(current) > 0) {
            continue;
        }
        visited.insert(current);

        const auto it = m_directChildren.find(current);
        if(it == m_directChildren.end()) {
            continue;
        }

        for(const auto& child_key : it->second) {
            switch(child_key.m_type) {
            case EntityType::Vertex:
                targets.m_nodes.insert(child_key);
                break;
            case EntityType::Edge:
                targets.m_edges.insert(child_key);
                break;
            case EntityType::Wire:
                targets.m_wires.insert(child_key);
                break;
            case EntityType::Face:
                targets.m_faces.insert(child_key);
                break;
            case EntityType::Shell:
                targets.m_shells.insert(child_key);
                break;
            case EntityType::Solid:
                targets.m_solids.insert(child_key);
                break;
            case EntityType::CompSolid:
                targets.m_compSolids.insert(child_key);
                break;
            case EntityType::Compound:
                targets.m_compounds.insert(child_key);
                break;
            case EntityType::Part:
                targets.m_parts.insert(child_key);
                break;
            default:
                break;
            }
            stack.push_back(child_key);
        }
    }
}

namespace {
template <class KeySet> std::vector<EntityId> toIds(const KeySet& keys) {
    std::vector<EntityId> ids;
    ids.reserve(keys.size());
    for(const auto& k : keys) {
        ids.push_back(k.m_id);
    }
    return ids;
}

template <class KeySet> std::vector<EntityKey> toKeys(const KeySet& keys) {
    std::vector<EntityKey> out;
    out.reserve(keys.size());
    for(const auto& k : keys) {
        out.push_back(k);
    }
    return out;
}
} // namespace

std::vector<EntityKey> EntityRelationshipIndex::selectByType(const RelatedTargets& targets,
                                                             EntityType target_type) {
    switch(target_type) {
    case EntityType::Vertex:
        return toKeys(targets.m_nodes);
    case EntityType::Edge:
        return toKeys(targets.m_edges);
    case EntityType::Wire:
        return toKeys(targets.m_wires);
    case EntityType::Face:
        return toKeys(targets.m_faces);
    case EntityType::Shell:
        return toKeys(targets.m_shells);
    case EntityType::Solid:
        return toKeys(targets.m_solids);
    case EntityType::CompSolid:
        return toKeys(targets.m_compSolids);
    case EntityType::Compound:
        return toKeys(targets.m_compounds);
    case EntityType::Part:
        return toKeys(targets.m_parts);
    default:
        break;
    }
    return {};
}

std::vector<EntityKey>
EntityRelationshipIndex::findDescendantsNoCache(const EntityKey& source,
                                                EntityType target_type) const {
    std::vector<EntityKey> result;

    std::unordered_set<EntityKey, EntityKeyHash> visited;
    std::queue<EntityKey> to_visit;

    const auto enqueue_direct_children = [this, &to_visit](const EntityKey& key) {
        const auto it = m_directChildren.find(key);
        if(it == m_directChildren.end()) {
            return;
        }
        for(const auto& child : it->second) {
            to_visit.push(child);
        }
    };

    enqueue_direct_children(source);

    while(!to_visit.empty()) {
        const EntityKey current = to_visit.front();
        to_visit.pop();
        if(visited.count(current) > 0) {
            continue;
        }
        visited.insert(current);

        if(current.m_type == target_type) {
            result.push_back(current);
        }

        enqueue_direct_children(current);
    }

    return result;
}

std::vector<EntityKey> EntityRelationshipIndex::findAncestorsNoCache(const EntityKey& source,
                                                                     EntityType target_type) const {
    std::vector<EntityKey> result;

    std::unordered_set<EntityKey, EntityKeyHash> visited;
    std::queue<EntityKey> to_visit;

    const auto seed_it = m_directParents.find(source);
    if(seed_it != m_directParents.end()) {
        for(const auto& parent : seed_it->second) {
            to_visit.push(parent);
        }
    }

    while(!to_visit.empty()) {
        const EntityKey current = to_visit.front();
        to_visit.pop();
        if(visited.count(current) > 0) {
            continue;
        }
        visited.insert(current);

        if(current.m_type == target_type) {
            result.push_back(current);
        }

        const auto up_it = m_directParents.find(current);
        if(up_it != m_directParents.end()) {
            for(const auto& parent : up_it->second) {
                to_visit.push(parent);
            }
        }
    }

    return result;
}

std::vector<EntityId> EntityRelationshipIndex::directChildren(EntityId parent_id) const {
    const auto parent = m_entityIndex.findById(parent_id);
    if(!parent) {
        return {};
    }
    return directChildren(*parent);
}

std::vector<EntityId> EntityRelationshipIndex::directParents(EntityId child_id) const {
    const auto child = m_entityIndex.findById(child_id);
    if(!child) {
        return {};
    }
    return directParents(*child);
}

std::vector<EntityId>
EntityRelationshipIndex::directChildren(const GeometryEntityImpl& parent) const {
    std::shared_lock lock(m_indexMutex);
    const auto it = m_directChildren.find(parent.entityKey());
    if(it == m_directChildren.end()) {
        return {};
    }
    return toIds(it->second);
}

std::vector<EntityId>
EntityRelationshipIndex::directParents(const GeometryEntityImpl& child) const {
    std::shared_lock lock(m_indexMutex);
    const auto it = m_directParents.find(child.entityKey());
    if(it == m_directParents.end()) {
        return {};
    }
    return toIds(it->second);
}

std::vector<EntityKey> EntityRelationshipIndex::findRelatedEntities(EntityId source_id,
                                                                    EntityType target_type) const {
    const auto source = m_entityIndex.findById(source_id);
    if(!source) {
        return {};
    }
    return findRelatedEntities(*source, target_type);
}

std::vector<EntityKey> EntityRelationshipIndex::findRelatedEntities(EntityUID source_uid,
                                                                    EntityType source_type,
                                                                    EntityType target_type) const {
    const auto source = m_entityIndex.findByUIDAndType(source_uid, source_type);
    if(!source) {
        return {};
    }
    return findRelatedEntities(*source, target_type);
}
std::vector<EntityKey>
EntityRelationshipIndex::findRelatedEntities(const GeometryEntityImpl& source,
                                             EntityType target_type) const {
    const EntityType source_type = source.entityType();
    if(source_type == EntityType::None) {
        return {};
    }
    if(target_type == EntityType::None) {
        return {};
    }

    const EntityKey source_key = source.entityKey();
    std::shared_lock lock(m_indexMutex);

    // Direction decision:
    // - Part: always treated as a container; queries are descendant-oriented.
    // - Other types: if target is "lower" in the topology chain, query descendants;
    //               otherwise query ancestors.
    const bool query_descendants =
        (source_type == EntityType::Part) ||
        (static_cast<uint8_t>(target_type) < static_cast<uint8_t>(source_type));

    if(query_descendants) {
        if(m_cacheValid) {
            const auto it = m_fullDescendants.find(source_key);
            if(it == m_fullDescendants.end()) {
                return {};
            }
            return selectByType(it->second, target_type);
        }
        return findDescendantsNoCache(source_key, target_type);
    }

    if(m_cacheValid) {
        const auto it = m_reverseEntityToRelatedTargets.find(source_key);
        if(it == m_reverseEntityToRelatedTargets.end()) {
            return {};
        }
        return selectByType(it->second, target_type);
    }
    return findAncestorsNoCache(source_key, target_type);
}
} // namespace OpenGeoLab::Geometry