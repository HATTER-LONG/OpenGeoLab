/**
 * @file render_pick_resolver.cpp
 * @brief PickResolver — priority-based entity selection and Wire/Part/Solid aggregate resolution.
 *
 * All pick-mode logic lives here:
 *  - Sub-entity (V/E/F) priority resolution
 *  - Wire aggregate resolution: face or nearest-face-edge → bounding wire
 *  - Part aggregate resolution: face/edge → parent part
 *  - Pick constraints (already-selected, wire-blocks-edge, etc.)
 */

#include "render_pick_resolver.hpp"
#include "render/render_select_manager.hpp"

#include <climits>

namespace OpenGeoLab::Render {

namespace {

[[nodiscard]] constexpr inline bool hasMask(RenderEntityTypeMask value, RenderEntityTypeMask mask) {
    return static_cast<uint32_t>(value & mask) != 0u;
}

/// Priority order for V/E/F sub-entity resolution (lower index = higher priority).
/// Wire/Solid/Part are NOT listed here — they are aggregate types resolved separately.
constexpr RenderEntityType PICK_PRIORITY[] = {
    RenderEntityType::Vertex,       RenderEntityType::MeshNode,   RenderEntityType::Edge,
    RenderEntityType::MeshLine,     RenderEntityType::Face,       RenderEntityType::Shell,
    RenderEntityType::MeshTriangle, RenderEntityType::MeshQuad4,  RenderEntityType::MeshTetra4,
    RenderEntityType::MeshHexa8,    RenderEntityType::MeshPrism6, RenderEntityType::MeshPyramid5,
};

constexpr int PICK_PRIORITY_COUNT =
    static_cast<int>(sizeof(PICK_PRIORITY) / sizeof(PICK_PRIORITY[0]));

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
// Bind pick data
// =============================================================================

void PickResolver::setPickData(const PickResolutionData& pick_data) { m_pickData = &pick_data; }

// =============================================================================
// Effective GPU pick mask
// =============================================================================

RenderEntityTypeMask PickResolver::computeEffectiveMask(RenderEntityTypeMask user_mask) const {
    RenderEntityTypeMask effective = user_mask;

    // Wire/Part modes must also render Face + Edge so we can resolve from sub-entities.
    if(hasMask(user_mask, RenderEntityTypeMask::Wire)) {
        effective = effective | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;
    }
    if(hasMask(user_mask, RenderEntityTypeMask::Part)) {
        effective = effective | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;
    }
    if(hasMask(user_mask, RenderEntityTypeMask::Solid)) {
        effective = effective | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;
    }
    return effective;
}

// =============================================================================
// Top-level dispatch
// =============================================================================

ResolvedPickResult PickResolver::resolve(const std::vector<uint64_t>& pick_ids,
                                         PickAction action,
                                         RenderEntityTypeMask user_mask) const {
    if(pick_ids.empty() || !m_pickData) {
        return ResolvedPickResult{};
    }

    if(hasMask(user_mask, RenderEntityTypeMask::Wire)) {
        return resolveWireMode(pick_ids, action);
    }
    if(hasMask(user_mask, RenderEntityTypeMask::Solid)) {
        return resolveSolidMode(pick_ids, action);
    }
    if(hasMask(user_mask, RenderEntityTypeMask::Part)) {
        return resolvePartMode(pick_ids, action);
    }
    return resolveVEFMode(pick_ids, action);
}

// =============================================================================
// Sub-entity (Vertex / Edge / Face) priority resolution
// =============================================================================

ResolvedPickResult PickResolver::resolveVEFMode(const std::vector<uint64_t>& pick_ids,
                                                PickAction action) const {
    uint64_t best_encoded = 0;
    int best_priority = INT_MAX;
    uint64_t best_face_encoded = 0;

    auto& select_mgr = RenderSelectManager::instance();

    for(const auto encoded : pick_ids) {
        const RenderEntityType type = PickId::decodeType(encoded);
        const auto uid = PickId::decodeUID(encoded);

        const auto part_it = m_pickData->m_entityToPartUid.find(encoded);
        const bool has_part = (part_it != m_pickData->m_entityToPartUid.end());
        const auto part_uid = has_part ? part_it->second : 0;

        // Check whether any wire containing this edge is currently selected.
        // If so: Add → blocked (wire takes precedence over edge);
        //        Remove → relevant (deselect via wire, not edge directly).
        bool edge_has_wire_selected = false;
        if(type == RenderEntityType::Edge) {
            auto wit = m_pickData->m_edgeToWireUids.find(uid);
            if(wit != m_pickData->m_edgeToWireUids.end()) {
                for(const auto wire_uid : wit->second) {
                    if(select_mgr.isWireSelected(wire_uid)) {
                        edge_has_wire_selected = true;
                        break;
                    }
                }
            }
        }

        const int priority = typePriority(type);

        if(action == PickAction::Add) {
            if(priority < best_priority && !select_mgr.isSelected({type, uid}) &&
               !(has_part && select_mgr.isPartSelected(part_uid)) && !edge_has_wire_selected) {
                best_priority = priority;
                best_encoded = encoded;
            }
        } else {
            // Remove: entity must be selected (directly, via part, or via wire)
            const bool relevant = select_mgr.isSelected({type, uid}) ||
                                  (has_part && select_mgr.isPartSelected(part_uid)) ||
                                  edge_has_wire_selected;
            if(priority < best_priority && relevant) {
                best_priority = priority;
                best_encoded = encoded;
            }
        }

        // Track first face for wire disambiguation
        if(type == RenderEntityType::Face && best_face_encoded == 0) {
            best_face_encoded = encoded;
        }
    }

    if(best_encoded == 0) {
        return ResolvedPickResult{};
    }

    ResolvedPickResult result;
    result.m_type = PickId::decodeType(best_encoded);
    result.m_uid = PickId::decodeUID(best_encoded);
    result.m_partUid = resolvePartUid(result.m_uid, result.m_type);
    result.m_faceContextUid = best_face_encoded != 0 ? PickId::decodeUID(best_face_encoded) : 0;
    result.m_solidUid = resolveSolidUid(result.m_uid, result.m_faceContextUid);
    if(result.m_type == RenderEntityType::Edge) {
        result.m_wireUid = resolveWireUid(result.m_uid, result.m_faceContextUid);
    }
    return result;
}

// =============================================================================
// Wire aggregate resolution
// =============================================================================
//
// Rules (per spec):
//   Hover/pick trigger: mouse is on a face OR on the boundary edge of the
//   nearest face.  In both cases we resolve to the Wire bounding that face.
//
//   Add:
//     - Wire already selected → skip (no duplicate).
//     - Some individual edges of this wire already selected → still allowed.
//     - Wire selected blocks individual edge picks (handled in VEF mode).
//
//   Remove:
//     - Wire is selected → allow (user can deselect the whole wire by
//       hovering the face / nearest-face-edge and right-clicking).
//     - Wire not selected → skip.
//
// =============================================================================

ResolvedPickResult PickResolver::resolveWireMode(const std::vector<uint64_t>& pick_ids,
                                                 PickAction action) const {
    uint64_t best_face_uid = 0;
    uint64_t best_edge_uid = 0;

    // Collect first face and first edge from the pick region (highest-occurrence
    // entity at center, but "first" is sufficient given small pick radius).
    for(const auto encoded : pick_ids) {
        const RenderEntityType type = PickId::decodeType(encoded);
        if(type == RenderEntityType::Face && best_face_uid == 0) {
            best_face_uid = PickId::decodeUID(encoded);
        } else if(type == RenderEntityType::Edge && best_edge_uid == 0) {
            best_edge_uid = PickId::decodeUID(encoded);
        }
        if(best_face_uid != 0 && best_edge_uid != 0) {
            break;
        }
    }

    // Resolve wire: prefer face-direct lookup (more precise), fall back to edge.
    uint64_t wire_uid = 0;
    if(best_face_uid != 0) {
        wire_uid = resolveWireUidForFace(best_face_uid);
    }
    if(wire_uid == 0 && best_edge_uid != 0) {
        wire_uid = resolveWireUid(best_edge_uid, best_face_uid);
    }

    if(wire_uid == 0) {
        return ResolvedPickResult{};
    }

    // Apply pick constraints.
    const bool is_selected = RenderSelectManager::instance().isWireSelected(wire_uid);
    if(action == PickAction::Add && is_selected) {
        return ResolvedPickResult{}; // already selected
    }
    if(action == PickAction::Remove && !is_selected) {
        return ResolvedPickResult{}; // nothing to remove
    }

    ResolvedPickResult result;
    result.m_uid = wire_uid;
    result.m_type = RenderEntityType::Wire;
    result.m_faceContextUid = best_face_uid;
    return result;
}

// =============================================================================
// Part aggregate resolution
// =============================================================================

ResolvedPickResult PickResolver::resolvePartMode(const std::vector<uint64_t>& pick_ids,
                                                 PickAction action) const {
    // Find any face or edge that has a known parent Part.
    uint64_t part_uid = 0;
    uint64_t face_context = 0;

    for(const auto encoded : pick_ids) {
        const RenderEntityType type = PickId::decodeType(encoded);
        if(type != RenderEntityType::Face && type != RenderEntityType::Edge) {
            continue;
        }

        auto it = m_pickData->m_entityToPartUid.find(encoded);
        if(it == m_pickData->m_entityToPartUid.end() || it->second == 0) {
            continue;
        }

        part_uid = it->second;
        if(type == RenderEntityType::Face) {
            face_context = PickId::decodeUID(encoded);
        }
        break; // first candidate is sufficient
    }

    if(part_uid == 0) {
        return ResolvedPickResult{};
    }

    const bool is_selected = RenderSelectManager::instance().isPartSelected(part_uid);
    if(action == PickAction::Add && is_selected) {
        return ResolvedPickResult{};
    }
    if(action == PickAction::Remove && !is_selected) {
        return ResolvedPickResult{};
    }

    ResolvedPickResult result;
    result.m_uid = part_uid;
    result.m_type = RenderEntityType::Part;
    result.m_partUid = part_uid;
    result.m_faceContextUid = face_context;
    return result;
}

ResolvedPickResult PickResolver::resolveSolidMode(const std::vector<uint64_t>& pick_ids,
                                                  PickAction action) const {
    uint64_t best_face_uid = 0;
    uint64_t best_edge_uid = 0;

    for(const auto encoded : pick_ids) {
        const RenderEntityType type = PickId::decodeType(encoded);
        if(type == RenderEntityType::Face && best_face_uid == 0) {
            best_face_uid = PickId::decodeUID(encoded);
        } else if(type == RenderEntityType::Edge && best_edge_uid == 0) {
            best_edge_uid = PickId::decodeUID(encoded);
        }
        if(best_face_uid != 0 && best_edge_uid != 0) {
            break;
        }
    }

    uint64_t solid_uid = 0;
    if(best_face_uid != 0) {
        solid_uid = resolveSolidUidForFace(best_face_uid);
    }
    if(solid_uid == 0 && best_edge_uid != 0) {
        solid_uid = resolveSolidUid(best_edge_uid, best_face_uid);
    }

    if(solid_uid == 0) {
        return ResolvedPickResult{};
    }

    const bool is_selected = RenderSelectManager::instance().isSolidSelected(solid_uid);
    if(action == PickAction::Add && is_selected) {
        return ResolvedPickResult{};
    }
    if(action == PickAction::Remove && !is_selected) {
        return ResolvedPickResult{};
    }

    ResolvedPickResult result;
    result.m_uid = solid_uid;
    result.m_type = RenderEntityType::Solid;
    result.m_solidUid = solid_uid;
    result.m_faceContextUid = best_face_uid;
    if(best_face_uid != 0) {
        result.m_partUid = resolvePartUid(best_face_uid, RenderEntityType::Face);
    } else if(best_edge_uid != 0) {
        result.m_partUid = resolvePartUid(best_edge_uid, RenderEntityType::Edge);
    }
    return result;
}

// =============================================================================
// Wire edge lookup (for highlight pass)
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
// Hierarchy helpers
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
    if(it->second.size() == 1) {
        return it->second[0];
    }
    // Multiple wires share this edge — prefer the wire whose face matches context.
    if(face_uid != 0) {
        for(const auto wire_uid : it->second) {
            auto fit = m_pickData->m_wireToFaceUid.find(wire_uid);
            if(fit != m_pickData->m_wireToFaceUid.end() && fit->second == face_uid) {
                return wire_uid;
            }
        }
    }
    return it->second[0];
}

uint64_t PickResolver::resolveWireUidForFace(uint64_t face_uid) const {
    if(!m_pickData || face_uid == 0) {
        return 0;
    }
    // Scan wireToFaceUid to find a wire whose bounding face is face_uid.
    // Typical geometry has few wires so linear scan is acceptable.
    for(const auto& [wire_uid, fuid] : m_pickData->m_wireToFaceUid) {
        if(fuid == face_uid) {
            return wire_uid;
        }
    }
    return 0;
}

uint64_t PickResolver::resolveSolidUid(uint64_t edge_uid, uint64_t face_uid) const {
    if(!m_pickData) {
        return 0;
    }
    if(face_uid != 0) {
        auto fit = m_pickData->m_faceToSolidUid.find(face_uid);
        if(fit != m_pickData->m_faceToSolidUid.end()) {
            return fit->second;
        }
    }

    auto it = m_pickData->m_edgeToSolidUids.find(edge_uid);
    if(it == m_pickData->m_edgeToSolidUids.end() || it->second.empty()) {
        return 0;
    }
    return it->second.front();
}

uint64_t PickResolver::resolveSolidUidForFace(uint64_t face_uid) const {
    if(!m_pickData || face_uid == 0) {
        return 0;
    }
    auto it = m_pickData->m_faceToSolidUid.find(face_uid);
    return it != m_pickData->m_faceToSolidUid.end() ? it->second : 0;
}

} // namespace OpenGeoLab::Render
