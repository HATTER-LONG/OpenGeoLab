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
    return static_cast<uint8_t>(value & mask) != 0u;
}
} // namespace

SelectManager& SelectManager::instance() {
    static SelectManager s_instance;
    return s_instance;
}

SelectManager::SelectManager() {
    m_pickTypes = PickTypes::Vertex | PickTypes::Edge | PickTypes::Face;
    LOG_TRACE("SelectManager created");
}

SelectManager::~SelectManager() { LOG_TRACE("SelectManager destroyed"); }

void SelectManager::setPickEnabled(bool enabled) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_pickEnabled == enabled) {
            return;
        }
        m_pickEnabled = enabled;
    }
    m_pickSettingsChanged.emitSignal();
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

    m_pickSettingsChanged.emitSignal();
}

SelectManager::PickTypes SelectManager::pickTypes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pickTypes;
}

bool SelectManager::isTypePickable(Geometry::EntityType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    switch(type) {
    case Geometry::EntityType::Vertex:
        return hasAny(m_pickTypes, PickTypes::Vertex);
    case Geometry::EntityType::Edge:
        return hasAny(m_pickTypes, PickTypes::Edge);
    case Geometry::EntityType::Face:
        return hasAny(m_pickTypes, PickTypes::Face);
    case Geometry::EntityType::Solid:
        return hasAny(m_pickTypes, PickTypes::Solid);
    case Geometry::EntityType::Part:
        return hasAny(m_pickTypes, PickTypes::Part);
    default:
        return false;
    }
}

bool SelectManager::addSelection(Geometry::EntityUID uid, Geometry::EntityType type) {
    if(!isValidSelection(uid, type)) {
        return false;
    }

    bool added = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const PickResult pr{type, uid};
        if(m_selections.find(pr) == m_selections.end()) {
            m_selections.insert(pr);
            added = true;
        }
    }

    if(added) {
        m_selectionChanged.emitSignal();
    }

    return added;
}

bool SelectManager::removeSelection(Geometry::EntityUID uid, Geometry::EntityType type) {
    if(!isValidSelection(uid, type)) {
        return false;
    }

    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const PickResult pr{type, uid};
        if(m_selections.erase(pr) > 0) {
            removed = true;
        }
    }

    if(removed) {
        m_selectionChanged.emitSignal();
    }

    return removed;
}

bool SelectManager::containsSelection(Geometry::EntityUID uid, Geometry::EntityType type) const {
    if(!isValidSelection(uid, type)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const PickResult pr{type, uid};
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
        m_selectionChanged.emitSignal();
    }
}

Util::ScopedConnection SelectManager::subscribePickSettingsChanged(std::function<void()> callback) {
    return m_pickSettingsChanged.connect(std::move(callback));
}

Util::ScopedConnection SelectManager::subscribeSelectionChanged(std::function<void()> callback) {
    return m_selectionChanged.connect(std::move(callback));
}

SelectManager::PickTypes SelectManager::normalizePickTypes(PickTypes types) {
    // Exclusivity rules:
    // - If Solid or Part is set, they must be exclusive and clear all others.
    // - If any of Vertex/Edge/Face are set, Solid/Part must be cleared.

    const bool wants_solid = hasAny(types, PickTypes::Solid);
    const bool wants_part = hasAny(types, PickTypes::Part);

    if(wants_solid && !wants_part) {
        return PickTypes::Solid;
    }
    if(wants_part && !wants_solid) {
        return PickTypes::Part;
    }

    // Solid and Part both requested: prefer Part (explicit UI-level selection).
    if(wants_part && wants_solid) {
        return PickTypes::Part;
    }

    // Only V/E/F mask.
    constexpr PickTypes vef_mask = PickTypes::Vertex | PickTypes::Edge | PickTypes::Face;
    const PickTypes vef = types & vef_mask;
    if(vef == PickTypes::None) {
        // Default to Face to avoid a "no-pick" state when callers pass None.
        return PickTypes::Face;
    }
    return vef;
}

bool SelectManager::isValidSelection(Geometry::EntityUID uid, Geometry::EntityType type) {
    return (type != Geometry::EntityType::None) && (uid != Geometry::INVALID_ENTITY_UID);
}

} // namespace OpenGeoLab::Render
