#include "render/render_select_manager.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Render {
namespace {
[[nodiscard]] constexpr inline bool hasAny(PickMask value, PickMask mask) {
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

void RenderSelectManager::setPickTypes(PickMask types) {
    const PickMask normalized_types = normalizePickTypes(types);
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

PickMask RenderSelectManager::getPickTypes() const {
    std::lock_guard lock(m_mutex);
    return m_pickTypes;
}

bool RenderSelectManager::isTypePickable(PickEntityType type) const {
    std::lock_guard lock(m_mutex);
    const PickMask type_mask = toMask(type);
    return hasAny(m_pickTypes, type_mask);
}

PickMask RenderSelectManager::normalizePickTypes(PickMask types) {
    // Exclusivity rules:
    // - Wire/Solid/Part are exclusive and clear all others.
    // - Vertex/Edge/Face can be combined with each other.
    // - MeshNode/MeshElement are exclusive from all geometry types.
    // - MeshNode and MeshElement can be combined with each other.

    const bool wants_mesh_node =
        hasAny(types, PickMask::MeshNode) && !hasAny(m_pickTypes, PickMask::MeshNode);
    const bool wants_mesh_element =
        hasAny(types, PickMask::MeshElement) && !hasAny(m_pickTypes, PickMask::MeshElement);

    if(wants_mesh_node || wants_mesh_element) {
        auto result = PickMask::None;
        if(wants_mesh_node) {
            result = result | PickMask::MeshNode;
        }
        if(wants_mesh_element) {
            result = result | PickMask::MeshElement;
        }
        return result;
    }

    const bool wants_solid =
        hasAny(types, PickMask::Solid) && !hasAny(m_pickTypes, PickMask::Solid);
    const bool wants_part = hasAny(types, PickMask::Part) && !hasAny(m_pickTypes, PickMask::Part);
    const bool wants_wire = hasAny(types, PickMask::Wire) && !hasAny(m_pickTypes, PickMask::Wire);

    if(wants_part) {
        return PickMask::Part;
    }
    if(wants_solid) {
        return PickMask::Solid;
    }
    if(wants_wire) {
        return PickMask::Wire;
    }
    // Only V/E/F mask.
    constexpr auto vef_mask = PickMask::Vertex | PickMask::Edge | PickMask::Face;

    return types & vef_mask;
}

bool RenderSelectManager::addSelection(Geometry::EntityUID entity_uid, Geometry::EntityType type) {
    if(entity_uid == Geometry::INVALID_ENTITY_UID || type == Geometry::EntityType::None) {
        LOG_WARN("RenderSelectManager: Attempt to select invalid entity '{}:{}'",
                 Geometry::entityTypeToString(type).value(), entity_uid);
        return false;
    }
    auto pick_type = toPickEntityType(type);
    return modifySelection(SelectionChangeAction::Added, PickResult{entity_uid, pick_type});
}

bool RenderSelectManager::removeSelection(Geometry::EntityUID entity_uid,
                                          Geometry::EntityType type) {
    if(entity_uid == Geometry::INVALID_ENTITY_UID || type == Geometry::EntityType::None) {
        LOG_WARN("RenderSelectManager: Attempt to remove invalid entity '{}:{}'",
                 Geometry::entityTypeToString(type).value(), entity_uid);
        return false;
    }

    auto pick_type = toPickEntityType(type);

    return modifySelection(SelectionChangeAction::Removed, PickResult{entity_uid, pick_type});
}

bool RenderSelectManager::addSelection(Mesh::MeshElementUID element_id,
                                       Mesh::MeshElementType type) {
    if(element_id == Mesh::INVALID_MESH_ELEMENT_UID || type == Mesh::MeshElementType::Invalid) {
        LOG_WARN("RenderSelectManager: Attempt to select invalid mesh element '{}:{}'",
                 meshElementTypeToString(type).value(), element_id);
        return false;
    }
    Mesh::MeshElementRef key{element_id, type};

    return modifySelection(SelectionChangeAction::Added,
                           PickResult{element_id, PickEntityType::MeshElement, type});
}

bool RenderSelectManager::removeSelection(Mesh::MeshElementUID element_id,
                                          Mesh::MeshElementType type) {
    if(element_id == Mesh::INVALID_MESH_ELEMENT_UID || type == Mesh::MeshElementType::Invalid) {
        LOG_WARN("RenderSelectManager: Attempt to remove invalid mesh element '{}:{}'",
                 meshElementTypeToString(type).value(), element_id);
        return false;
    }

    Mesh::MeshElementRef key{element_id, type};
    return modifySelection(SelectionChangeAction::Removed,
                           PickResult{element_id, PickEntityType::MeshElement, type});
}

bool RenderSelectManager::addSelection(Mesh::MeshNodeId node_id) {
    if(node_id == Mesh::INVALID_MESH_NODE_ID) {
        LOG_WARN("RenderSelectManager: Attempt to select invalid mesh node '{}'", node_id);
        return false;
    }
    return modifySelection(SelectionChangeAction::Added,
                           PickResult{node_id, PickEntityType::MeshNode});
}

bool RenderSelectManager::removeSelection(Mesh::MeshNodeId node_id) {
    if(node_id == Mesh::INVALID_MESH_NODE_ID) {
        LOG_WARN("RenderSelectManager: Attempt to remove invalid mesh node '{}'", node_id);
        return false;
    }
    return modifySelection(SelectionChangeAction::Removed,
                           PickResult{node_id, PickEntityType::MeshNode});
}

bool RenderSelectManager::modifySelection(SelectionChangeAction action, PickResult pick_result) {
    if(!isTypePickable(pick_result.m_type)) {
        LOG_WARN("RenderSelectManager: Attempt to select unpickable type '{}'",
                 static_cast<int>(pick_result.m_type));
        return false;
    }
    if(action == SelectionChangeAction::Cleared) {
        LOG_WARN(
            "RenderSelectManager: Invalid selection change action cleared for modifySelection");
        return false;
    }

    {
        std::lock_guard lock(m_mutex);

        if(action == SelectionChangeAction::Added) {
            if(m_currentSelections.contains(pick_result)) {
                return false;
            }
            m_currentSelections.emplace(pick_result);
        } else {
            if(!m_currentSelections.contains(pick_result)) {
                return false;
            }
            m_currentSelections.erase(pick_result);
        }
    }

    m_selectionChanged.emitSignal(pick_result, action);
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