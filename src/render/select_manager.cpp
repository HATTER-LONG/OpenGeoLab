/**
 * @file select_manager.cpp
 * @brief Implementation of SelectManager
 */

#include "render/select_manager.hpp"

#include "util/logger.hpp"

namespace OpenGeoLab::Render {

namespace {
[[nodiscard]] constexpr inline bool hasAny(SelectManager::PickTypes value,
                                           SelectManager::PickTypes mask) {
    return static_cast<uint32_t>(value & mask) != 0u;
}
} // namespace

SelectManager& SelectManager::instance() {
    static SelectManager s_instance;
    return s_instance;
}

SelectManager::SelectManager() { LOG_TRACE("SelectManager created"); }

SelectManager::~SelectManager() { LOG_TRACE("SelectManager destroyed"); }

void SelectManager::setPickEnabled(bool enabled) {
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

bool SelectManager::isPickEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pickEnabled;
}

void SelectManager::setPickTypes(PickTypes types) {
    const PickTypes normalized = normalizePickTypes(types);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_pickTypes == normalized) {
            return;
        }
        m_pickTypes = normalized;
    }
    if(m_pickEnabled) {
        m_pickSettingsChanged.emitSignal(m_pickTypes);
    }
}

SelectManager::PickTypes SelectManager::pickTypes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pickTypes;
}

bool SelectManager::isTypePickable(RenderEntityType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    switch(type) {
    case RenderEntityType::Vertex:
        return hasAny(m_pickTypes, PickTypes::Vertex);
    case RenderEntityType::Edge:
        return hasAny(m_pickTypes, PickTypes::Edge);
    case RenderEntityType::Wire:
        return hasAny(m_pickTypes, PickTypes::Wire);
    case RenderEntityType::Face:
        return hasAny(m_pickTypes, PickTypes::Face);
    case RenderEntityType::Solid:
        return hasAny(m_pickTypes, PickTypes::Solid);
    case RenderEntityType::Part:
        return hasAny(m_pickTypes, PickTypes::Part);
    case RenderEntityType::MeshNode:
        return hasAny(m_pickTypes, PickTypes::MeshNode);
    case RenderEntityType::MeshElement:
        return hasAny(m_pickTypes, PickTypes::MeshElement);
    default:
        return false;
    }
}

bool SelectManager::addSelection(uint64_t uid56, RenderEntityType type) {
    if(!isValidSelection(uid56, type)) {
        return false;
    }

    bool added = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const PickResult pr{type, uid56};
        if(m_selections.find(pr) == m_selections.end()) {
            m_selections.insert(pr);
            added = true;
        }
    }

    if(added) {
        const PickResult pr{type, uid56};
        m_selectionChanged.emitSignal(pr, SelectionChangeAction::Added);
    }

    return added;
}

bool SelectManager::removeSelection(uint64_t uid56, RenderEntityType type) {
    if(!isValidSelection(uid56, type)) {
        return false;
    }

    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const PickResult pr{type, uid56};
        if(m_selections.erase(pr) > 0) {
            removed = true;
        }
    }

    if(removed) {
        const PickResult pr{type, uid56};
        m_selectionChanged.emitSignal(pr, SelectionChangeAction::Removed);
    }

    return removed;
}

bool SelectManager::containsSelection(uint64_t uid56, RenderEntityType type) const {
    if(!isValidSelection(uid56, type)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const PickResult pr{type, uid56};
    return m_selections.find(pr) != m_selections.end();
}

std::vector<SelectManager::PickResult> SelectManager::selections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<PickResult>(m_selections.begin(), m_selections.end());
}

void SelectManager::clearSelections() {
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_selections.empty()) {
            return;
        }
        m_selections.clear();
        changed = true;
    }

    if(changed) {
        m_selectionChanged.emitSignal(PickResult{}, SelectionChangeAction::Cleared);
    }
}

Util::ScopedConnection
SelectManager::subscribePickSettingsChanged(std::function<void(PickTypes)> callback) {
    return m_pickSettingsChanged.connect(std::move(callback));
}

Util::ScopedConnection
SelectManager::subscribePickEnabledChanged(std::function<void(bool)> callback) {
    return m_pickEnabledChanged.connect(std::move(callback));
}

Util::ScopedConnection SelectManager::subscribeSelectionChanged(
    std::function<void(PickResult, SelectionChangeAction)> callback) {
    return m_selectionChanged.connect(std::move(callback));
}

SelectManager::PickTypes SelectManager::normalizePickTypes(PickTypes types) {
    // Exclusivity rules:
    // - Wire/Solid/Part are exclusive and clear all others.
    // - Vertex/Edge/Face can be combined with each other.
    // - MeshNode/MeshElement are exclusive from all geometry types.
    // - MeshNode and MeshElement can be combined with each other.

    const bool wants_mesh_node = hasAny(types, PickTypes::MeshNode);
    const bool wants_mesh_elem = hasAny(types, PickTypes::MeshElement);

    // Mesh types take priority and exclude all geometry types.
    if(wants_mesh_node || wants_mesh_elem) {
        PickTypes result = PickTypes::None;
        if(wants_mesh_node) {
            result = result | PickTypes::MeshNode;
        }
        if(wants_mesh_elem) {
            result = result | PickTypes::MeshElement;
        }
        return result;
    }

    const bool wants_solid = hasAny(types, PickTypes::Solid);
    const bool wants_part = hasAny(types, PickTypes::Part);
    const bool wants_wire = hasAny(types, PickTypes::Wire);

    if(wants_part) {
        return PickTypes::Part;
    }
    if(wants_solid) {
        return PickTypes::Solid;
    }
    if(wants_wire) {
        return PickTypes::Wire;
    }

    // Only V/E/F mask.
    constexpr PickTypes vef_mask = PickTypes::Vertex | PickTypes::Edge | PickTypes::Face;
    return types & vef_mask;
}

bool SelectManager::isValidSelection(uint64_t uid56, RenderEntityType type) {
    return (type != RenderEntityType::None) && (uid56 != 0);
}

} // namespace OpenGeoLab::Render
