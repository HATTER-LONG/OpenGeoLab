#include "pick_resolver.hpp"

#include <opengeolab/scene/pick_id.hpp>

#include <unordered_set>

namespace OpenGeoLab::Render {

PickResolver::PickResolver(const Scene::TopologyIndex& topo_index) : m_topoIndex(topo_index) {}

int PickResolver::typePriority(Core::EntityType type) {
    switch(type) {
    case Core::EntityType::GeoVertex:
        return 3;
    case Core::EntityType::GeoEdge:
        return 2;
    case Core::EntityType::GeoWire:
        return 2;
    case Core::EntityType::GeoFace:
        return 1;
    case Core::EntityType::GeoSolid:
        return 1;
    default:
        return 0;
    }
}

PickResult PickResolver::resolveOne(uint64_t pick_id, PickMode mode) const {
    if(!Scene::PickId::isValid(pick_id)) {
        return {};
    }

    const uint32_t shape_id = Scene::PickId::decodeShapeId(pick_id);
    const Core::EntityType type = Scene::PickId::decodeType(pick_id);
    const uint32_t local_id = Scene::PickId::decodeLocalId(pick_id);

    PickResult result{shape_id, type, local_id, true};

    switch(mode) {
    case PickMode::Part:
        result.entityType = Core::EntityType::GeoSolid;
        result.localId = 1;
        break;
    case PickMode::Wire:
        if(type == Core::EntityType::GeoEdge) {
            if(auto wire = m_topoIndex.edgeToWire(shape_id, local_id)) {
                result.entityType = Core::EntityType::GeoWire;
                result.localId = *wire;
            } else {
                // Fallback: first wire (1-based, EntityRef requires localId != 0).
                result.entityType = Core::EntityType::GeoWire;
                result.localId = 1;
            }
        } else {
            result.entityType = Core::EntityType::GeoWire;
            result.localId = 1;
        }
        break;
    case PickMode::Solid:
        if(type == Core::EntityType::GeoFace) {
            if(auto solid = m_topoIndex.faceToSolid(shape_id, local_id)) {
                result.entityType = Core::EntityType::GeoSolid;
                result.localId = *solid;
            } else {
                // Fallback: first solid (1-based, EntityRef requires localId != 0).
                result.entityType = Core::EntityType::GeoSolid;
                result.localId = 1;
            }
        } else {
            result.entityType = Core::EntityType::GeoSolid;
            result.localId = 1;
        }
        break;
    case PickMode::VEF:
        break;
    }

    return result;
}

PickResult PickResolver::resolve(const std::vector<uint64_t>& raw_pick_ids, PickMode mode) const {
    PickResult best;
    int best_priority = -1;

    for(const uint64_t id : raw_pick_ids) {
        auto candidate = resolveOne(id, mode);
        if(!candidate.valid) {
            continue;
        }

        if(mode == PickMode::Part || mode == PickMode::Wire || mode == PickMode::Solid) {
            return candidate;
        }

        const int prio = typePriority(candidate.entityType);
        if(prio > best_priority) {
            best = candidate;
            best_priority = prio;
        }
    }

    return best;
}

std::vector<PickResult> PickResolver::resolveAll(const std::vector<uint64_t>& raw_pick_ids,
                                                 PickMode mode) const {
    std::vector<PickResult> results;
    std::unordered_set<uint64_t> seen;

    for(const uint64_t id : raw_pick_ids) {
        if(!Scene::PickId::isValid(id) || seen.count(id) > 0) {
            continue;
        }
        seen.insert(id);

        auto result = resolveOne(id, mode);
        if(result.valid) {
            results.push_back(result);
        }
    }

    return results;
}

} // namespace OpenGeoLab::Render
