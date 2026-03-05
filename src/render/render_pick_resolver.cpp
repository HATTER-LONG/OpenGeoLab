/**
 * @file pick_resolver.cpp
 * @brief PickResolver implementation — priority-based entity selection
 *        and Part/Wire hierarchy resolution from raw GPU pick IDs.
 */

#include "render_pick_resolver.hpp"

#include <climits>

namespace OpenGeoLab::Render {

namespace {

/// Priority order for pick type resolution (lower index = higher priority)
constexpr RenderEntityType PICK_PRIORITY[] = {
    RenderEntityType::Vertex,       RenderEntityType::MeshNode,   RenderEntityType::Edge,
    RenderEntityType::MeshLine,     RenderEntityType::Face,       RenderEntityType::Shell,
    RenderEntityType::Wire,         RenderEntityType::Solid,      RenderEntityType::Part,
    RenderEntityType::MeshTriangle, RenderEntityType::MeshQuad4,  RenderEntityType::MeshTetra4,
    RenderEntityType::MeshHexa8,    RenderEntityType::MeshPrism6, RenderEntityType::MeshPyramid5,
};

constexpr int PICK_PRIORITY_COUNT = sizeof(PICK_PRIORITY) / sizeof(PICK_PRIORITY[0]);

/// Returns priority index for a type (lower = higher priority). Returns INT_MAX if not found.
int typePriority(RenderEntityType type) {
    for(int i = 0; i < PICK_PRIORITY_COUNT; ++i) {
        if(PICK_PRIORITY[i] == type) {
            return i;
        }
    }
    return INT_MAX;
}

} // anonymous namespace

// =============================================================================
// Set pick data reference
// =============================================================================

void PickResolver::setPickData(const PickResolutionData& pick_data) { m_pickData = &pick_data; }

// =============================================================================
// Priority-based entity resolution
// =============================================================================

ResolvedPickResult PickResolver::resolve(const std::vector<uint64_t>& pick_ids) const {
    ResolvedPickResult result;

    uint64_t best_encoded = 0;
    int best_priority = INT_MAX;
    uint64_t best_face_encoded = 0;

    for(const auto encoded : pick_ids) {
        const RenderEntityType type = PickId::decodeType(encoded);
        const int priority = typePriority(type);
        if(priority < best_priority) {
            best_priority = priority;
            best_encoded = encoded;
        }
        // Track the first Face as context for wire resolution
        if(type == RenderEntityType::Face && best_face_encoded == 0) {
            best_face_encoded = encoded;
        }
    }

    if(best_encoded == 0) {
        return result;
    }

    result.m_type = PickId::decodeType(best_encoded);
    result.m_uid = PickId::decodeUID(best_encoded);
    result.m_partUid = resolvePartUid(result.m_uid, result.m_type);
    result.m_faceContextUid = best_face_encoded != 0 ? PickId::decodeUID(best_face_encoded) : 0;

    // Resolve wire for edges (using face context for disambiguation)
    if(result.m_type == RenderEntityType::Edge) {
        result.m_wireUid = resolveWireUid(result.m_uid, result.m_faceContextUid);
    }

    return result;
}

// =============================================================================
// Wire edge lookup
// =============================================================================

const std::vector<uint64_t>& PickResolver::wireEdges(uint64_t wire_uid) const {
    static const std::vector<uint64_t> empty;
    if(!m_pickData) {
        return empty;
    }
    auto it = m_pickData->m_wireToEdgeUids.find(wire_uid);
    return it != m_pickData->m_wireToEdgeUids.end() ? it->second : empty;
}

void PickResolver::clear() { m_pickData = nullptr; }

// =============================================================================
// Private hierarchy resolution
// =============================================================================

uint64_t PickResolver::resolvePartUid(uint64_t uid, RenderEntityType type) const {
    if(type == RenderEntityType::Part) {
        return uid;
    }
    if(!m_pickData) {
        return 0;
    }
    const uint64_t encoded = PickId::encode(type, uid);
    auto it = m_pickData->m_entityToPartUid.find(encoded);
    return it != m_pickData->m_entityToPartUid.end() ? it->second : 0;
}

uint64_t PickResolver::resolveWireUid(uint64_t edge_uid, uint64_t face_uid) const {
    if(!m_pickData) {
        return 0;
    }
    auto it = m_pickData->m_edgeToWireUids.find(edge_uid);
    if(it == m_pickData->m_edgeToWireUids.end() || it->second.empty()) {
        return 0;
    }

    // If there's only one wire, return it directly
    if(it->second.size() == 1) {
        return it->second[0];
    }

    // Multiple wires share this edge — prefer the wire belonging to the given face
    if(face_uid != 0) {
        for(const auto wire_uid : it->second) {
            auto fit = m_pickData->m_wireToFaceUid.find(wire_uid);
            if(fit != m_pickData->m_wireToFaceUid.end() && fit->second == face_uid) {
                return wire_uid;
            }
        }
    }

    // Fallback: return the first wire
    return it->second[0];
}

} // namespace OpenGeoLab::Render
