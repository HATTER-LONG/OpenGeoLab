/**
 * @file render_select_manager.cpp
 * @brief RenderSelectManager — thread-safe selection/hover state with
 *        pick-type exclusivity rules (e.g. Wire/Solid/Part are mutually
 *        exclusive, Vertex/Edge/Face can combine).
 */

#include "render/render_select_manager.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
namespace {
[[nodiscard]] constexpr inline bool hasAny(RenderEntityTypeMask value, RenderEntityTypeMask mask) {
    return static_cast<uint32_t>(value & mask) != 0u;
}
} // namespace
RenderSelectManager& RenderSelectManager::instance() {
    static RenderSelectManager instance;
    return instance;
}

void RenderSelectManager::setPickEnabled(bool enabled) {
    bool notify_enabled = false;
    bool notify_types = false;
    RenderEntityTypeMask types{};
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_pickEnabled == enabled) {
            return;
        }
        m_pickEnabled = enabled;
        notify_enabled = true;
        if(m_pickEnabled) {
            notify_types = true;
            types = m_pickTypes;
        }
    }
    if(notify_enabled) {
        m_pickEnabledChanged.emitSignal(enabled);
    }
    if(notify_types) {
        m_pickSettingsChanged.emitSignal(types);
    }
}
bool RenderSelectManager::isPickEnabled() const {
    std::lock_guard lock(m_mutex);
    return m_pickEnabled;
}

void RenderSelectManager::setPickTypes(RenderEntityTypeMask types) {
    const RenderEntityTypeMask normalized_types = normalizePickTypes(types);
    bool should_notify = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_pickTypes == normalized_types) {
            return;
        }
        m_pickTypes = normalized_types;
        should_notify = m_pickEnabled;
    }

    if(should_notify) {
        m_pickSettingsChanged.emitSignal(normalized_types);
    }
}

RenderEntityTypeMask RenderSelectManager::getPickTypes() const {
    std::lock_guard lock(m_mutex);
    return m_pickTypes;
}

bool RenderSelectManager::isTypePickable(RenderEntityType type) const {
    std::lock_guard lock(m_mutex);
    const RenderEntityTypeMask type_mask = toMask(type);
    return hasAny(m_pickTypes, type_mask);
}

RenderEntityTypeMask RenderSelectManager::normalizePickTypes(RenderEntityTypeMask types) {
    // Exclusivity rules:
    // - Wire/Solid/Part are exclusive and clear all others.
    // - Vertex/Edge/Face can be combined with each other.
    // - MeshNode/MeshElement are exclusive from all geometry types.
    // - MeshNode and MeshElement can be combined with each other.

    const bool wants_mesh_node = hasAny(types, RenderEntityTypeMask::MeshNode) &&
                                 !hasAny(m_pickTypes, RenderEntityTypeMask::MeshNode);
    const bool wants_mesh_line = hasAny(types, RenderEntityTypeMask::MeshLine) &&
                                 !hasAny(m_pickTypes, RenderEntityTypeMask::MeshLine);
    const bool wants_mesh_element =
        hasAny(types, RENDER_MESH_ELEMENTS) && !hasAny(m_pickTypes, RENDER_MESH_ELEMENTS);

    if(wants_mesh_node || wants_mesh_line || wants_mesh_element) {
        auto result = RenderEntityTypeMask::None;
        if(wants_mesh_node) {
            result = result | RenderEntityTypeMask::MeshNode;
        }
        if(wants_mesh_line) {
            result = result | RenderEntityTypeMask::MeshLine;
        }
        if(wants_mesh_element) {
            result = result | RENDER_MESH_ELEMENTS;
        }
        return result;
    }

    const bool wants_solid = hasAny(types, RenderEntityTypeMask::Solid) &&
                             !hasAny(m_pickTypes, RenderEntityTypeMask::Solid);
    const bool wants_part = hasAny(types, RenderEntityTypeMask::Part) &&
                            !hasAny(m_pickTypes, RenderEntityTypeMask::Part);
    const bool wants_wire = hasAny(types, RenderEntityTypeMask::Wire) &&
                            !hasAny(m_pickTypes, RenderEntityTypeMask::Wire);

    if(wants_part) {
        return RenderEntityTypeMask::Part;
    }
    if(wants_solid) {
        return RenderEntityTypeMask::Solid;
    }
    if(wants_wire) {
        return RenderEntityTypeMask::Wire;
    }
    // Only V/E/F mask.
    constexpr auto vef_mask =
        RenderEntityTypeMask::Vertex | RenderEntityTypeMask::Edge | RenderEntityTypeMask::Face;

    return types & vef_mask;
}

bool RenderSelectManager::addSelection(uint64_t entity_uid, RenderEntityType type) {
    if(type == RenderEntityType::None) {
        LOG_WARN("RenderSelectManager: Attempt to select invalid entity '{}:{}'",
                 static_cast<int>(type), entity_uid);
        return false;
    }
    if(!isTypePickable(type)) {
        LOG_WARN("RenderSelectManager: Attempt to select unpickable type '{}'",
                 static_cast<int>(type));
        return false;
    }
    PickResult pick_result{entity_uid, type};
    {
        std::lock_guard lock(m_mutex);
        if(m_currentSelections.contains(pick_result)) {
            return false;
        }
        m_currentSelections.emplace(pick_result);
    }

    m_selectionChanged.emitSignal(pick_result, SelectionChangeAction::Added);
    return true;
}

bool RenderSelectManager::removeSelection(uint64_t entity_uid, RenderEntityType type) {
    if(entity_uid == Geometry::INVALID_ENTITY_UID || type == RenderEntityType::None) {
        LOG_WARN("RenderSelectManager: Attempt to remove invalid entity '{}:{}'",
                 static_cast<int>(type), entity_uid);
        return false;
    }

    PickResult pick_result{entity_uid, type};
    {
        std::lock_guard lock(m_mutex);
        if(!m_currentSelections.contains(pick_result)) {
            return false;
        }
        m_currentSelections.erase(pick_result);
    }
    m_selectionChanged.emitSignal(pick_result, SelectionChangeAction::Removed);
    return true;
}

void RenderSelectManager::clearSelection() {
    {
        std::lock_guard lock(m_mutex);
        m_currentSelections.clear();
        m_selectedWireEdges.clear();
    }
    m_selectionChanged.emitSignal(PickResult{}, SelectionChangeAction::Cleared);
}

std::vector<PickResult> RenderSelectManager::selections() const {
    std::vector<PickResult> out;

    std::lock_guard lock(m_mutex);

    out.reserve(m_currentSelections.size());

    for(const auto& s : m_currentSelections) {
        out.push_back(s);
    }

    return out;
}

// =============================================================================
// Hover state
// =============================================================================

void RenderSelectManager::setHoverEntity(uint64_t uid,
                                         RenderEntityType type,
                                         uint64_t part_uid,
                                         uint64_t wire_uid) {
    bool changed = false;
    {
        std::lock_guard lock(m_mutex);
        if(m_hoveredEntity.m_uid != uid || m_hoveredEntity.m_type != type ||
           m_hoveredPartUid != part_uid || m_hoveredWireUid != wire_uid) {
            m_hoveredEntity = PickResult{uid, type};
            m_hoveredPartUid = part_uid;
            m_hoveredWireUid = wire_uid;
            changed = true;
        }
    }
    if(changed) {
        m_hoverChanged.emitSignal();
    }
}

void RenderSelectManager::clearHover() {
    bool changed = false;
    {
        std::lock_guard lock(m_mutex);
        if(m_hoveredEntity.m_uid != 0 || m_hoveredEntity.m_type != RenderEntityType::None) {
            m_hoveredEntity = PickResult{0, RenderEntityType::None};
            m_hoveredPartUid = 0;
            m_hoveredWireUid = 0;
            m_hoveredWireEdgeUids.clear();
            changed = true;
        }
    }
    if(changed) {
        m_hoverChanged.emitSignal();
    }
}

bool RenderSelectManager::isEntityHovered(const RenderNodeKey& key) const {
    std::lock_guard lock(m_mutex);
    return m_hoveredEntity.m_uid == key.m_uid && m_hoveredEntity.m_type == key.m_type;
}

PickResult RenderSelectManager::hoveredEntity() const {
    std::lock_guard lock(m_mutex);
    return m_hoveredEntity;
}

bool RenderSelectManager::isPartHovered(uint64_t part_uid) const {
    if(part_uid == 0) {
        return false;
    }
    std::lock_guard lock(m_mutex);
    return m_hoveredPartUid == part_uid;
}

bool RenderSelectManager::isWireHovered(uint64_t wire_uid) const {
    if(wire_uid == 0) {
        return false;
    }
    std::lock_guard lock(m_mutex);
    return m_hoveredWireUid == wire_uid;
}

// =============================================================================
// Selection query
// =============================================================================

bool RenderSelectManager::isSelected(const RenderNodeKey& key) const {
    std::lock_guard lock(m_mutex);
    return m_currentSelections.contains(PickResult{key.m_uid, key.m_type});
}

bool RenderSelectManager::isPartSelected(uint64_t part_uid) const {
    if(part_uid == 0) {
        return false;
    }
    std::lock_guard lock(m_mutex);
    return m_currentSelections.contains(PickResult{part_uid, RenderEntityType::Part});
}

bool RenderSelectManager::isWireSelected(uint64_t wire_uid) const {
    if(wire_uid == 0) {
        return false;
    }
    std::lock_guard lock(m_mutex);
    return m_currentSelections.contains(PickResult{wire_uid, RenderEntityType::Wire});
}

// =============================================================================
// Wire edge tracking for complete wire highlighting
// =============================================================================

void RenderSelectManager::setHoveredWireEdges(const std::vector<uint64_t>& edge_uids) {
    std::lock_guard lock(m_mutex);
    m_hoveredWireEdgeUids.clear();
    m_hoveredWireEdgeUids.insert(edge_uids.begin(), edge_uids.end());
}
bool RenderSelectManager::isEdgeInHoveredWire(uint64_t edge_uid) const {
    if(edge_uid == 0) {
        return false;
    }
    std::lock_guard lock(m_mutex);
    return m_hoveredWireEdgeUids.contains(edge_uid);
}
void RenderSelectManager::addSelectedWireEdges(uint64_t wire_uid,
                                               const std::vector<uint64_t>& edge_uids) {
    std::lock_guard lock(m_mutex);
    m_selectedWireEdges[wire_uid].insert(edge_uids.begin(), edge_uids.end());
}
void RenderSelectManager::removeSelectedWireEdges(uint64_t wire_uid) {
    std::lock_guard lock(m_mutex);
    m_selectedWireEdges.erase(wire_uid);
}
void RenderSelectManager::clearSelectedWireEdges() {
    std::lock_guard lock(m_mutex);
    m_selectedWireEdges.clear();
}
bool RenderSelectManager::isEdgeInSelectedWire(uint64_t edge_uid) const {
    if(edge_uid == 0) {
        return false;
    }
    std::lock_guard lock(m_mutex);
    for(const auto& [_, edges] : m_selectedWireEdges) {
        if(edges.contains(edge_uid)) {
            return true;
        }
    }
    return false;
}
} // namespace OpenGeoLab::Render