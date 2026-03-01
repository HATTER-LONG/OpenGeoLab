/**
 * @file pick_resolver.cpp
 * @brief PickResolver implementation — priority-based entity selection
 *        and Part/Wire hierarchy resolution from raw GPU pick IDs.
 */

#include "render/pick_resolver.hpp"

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

void PickResolver::setPickData(const PickResolutionData& pickData) { m_pickData = &pickData; }

// =============================================================================
// Priority-based entity resolution
// =============================================================================

ResolvedPickResult PickResolver::resolve(const std::vector<uint64_t>& pickIds) const {
    ResolvedPickResult result;

    uint64_t bestEncoded = 0;
    int bestPriority = INT_MAX;
    uint64_t bestFaceEncoded = 0;

    for(const auto encoded : pickIds) {
        const RenderEntityType type = PickId::decodeType(encoded);
        const int priority = typePriority(type);
        if(priority < bestPriority) {
            bestPriority = priority;
            bestEncoded = encoded;
        }
        // Track the first Face as context for wire resolution
        if(type == RenderEntityType::Face && bestFaceEncoded == 0) {
            bestFaceEncoded = encoded;
        }
    }

    if(bestEncoded == 0) {
        return result;
    }

    result.type = PickId::decodeType(bestEncoded);
    result.uid = PickId::decodeUID(bestEncoded);
    result.partUid = resolvePartUid(result.uid, result.type);
    result.faceContextUid = bestFaceEncoded != 0 ? PickId::decodeUID(bestFaceEncoded) : 0;

    // Resolve wire for edges (using face context for disambiguation)
    if(result.type == RenderEntityType::Edge) {
        result.wireUid = resolveWireUid(result.uid, result.faceContextUid);
    }

    return result;
}

// =============================================================================
// Wire edge lookup
// =============================================================================

const std::vector<uint64_t>& PickResolver::wireEdges(uint64_t wireUid) const {
    static const std::vector<uint64_t> empty;
    if(!m_pickData) {
        return empty;
    }
    auto it = m_pickData->m_wireToEdgeUids.find(wireUid);
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
    auto it = m_pickData->m_entityToPartUid.find(uid);
    return it != m_pickData->m_entityToPartUid.end() ? it->second : 0;
}

uint64_t PickResolver::resolveWireUid(uint64_t edgeUid, uint64_t faceUid) const {
    if(!m_pickData) {
        return 0;
    }
    auto it = m_pickData->m_edgeToWireUids.find(edgeUid);
    if(it == m_pickData->m_edgeToWireUids.end() || it->second.empty()) {
        return 0;
    }

    // If there's only one wire, return it directly
    if(it->second.size() == 1) {
        return it->second[0];
    }

    // Multiple wires share this edge — prefer the wire belonging to the given face
    if(faceUid != 0) {
        for(const auto wire_uid : it->second) {
            auto fit = m_pickData->m_wireToFaceUid.find(wire_uid);
            if(fit != m_pickData->m_wireToFaceUid.end() && fit->second == faceUid) {
                return wire_uid;
            }
        }
    }

    // Fallback: return the first wire
    return it->second[0];
}

} // namespace OpenGeoLab::Render
