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
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_pickEnabled == enabled) {
            return;
        }
        m_pickEnabled = enabled;
    }
    // Notify listeners about enabled state change.
    m_pickEnabledChanged.emitSignal(m_pickEnabled);

    // If picking is enabled, notify pick type changes as well so UI can update.
    if(m_pickEnabled) {
        m_pickSettingsChanged.emitSignal(m_pickTypes);
    }
}

bool RenderSelectManager::isPickEnabled() const {
    std::lock_guard lock(m_mutex);
    return m_pickEnabled;
}

void RenderSelectManager::setPickTypes(RenderEntityTypeMask types) {
    const RenderEntityTypeMask normalized_types = normalizePickTypes(types);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_pickTypes == normalized_types) {
            return;
        }
        m_pickTypes = normalized_types;
    }

    if(m_pickEnabled) {
        m_pickSettingsChanged.emitSignal(m_pickTypes);
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
} // namespace OpenGeoLab::Render