/**
 * @file selection_state.cpp
 * @brief SelectionState implementation
 */

#include <opengeolab/scene/selection_state.hpp>

#include <algorithm>

namespace OpenGeoLab::Scene {

SelectionState::SelectionState() = default;
SelectionState::~SelectionState() = default;

void SelectionState::setPickEnabled(bool enabled) {
    {
        std::unique_lock lock(m_mutex);
        if(m_pickEnabled == enabled) {
            return;
        }
        m_pickEnabled = enabled;
    }
    pickConfigChanged.emit();
}

bool SelectionState::pickEnabled() const {
    std::shared_lock lock(m_mutex);
    return m_pickEnabled;
}

void SelectionState::setPickMask(Core::PickMask mask) {
    {
        std::unique_lock lock(m_mutex);
        if(m_pickMask == mask) {
            return;
        }
        m_pickMask = mask;
    }
    pickConfigChanged.emit();
}

Core::PickMask SelectionState::pickMask() const {
    std::shared_lock lock(m_mutex);
    return m_pickMask;
}

void SelectionState::addSelection(const Core::EntityRef& entity) {
    if(!entity.isValid()) {
        return;
    }
    {
        std::unique_lock lock(m_mutex);
        const auto it = std::lower_bound(m_selections.begin(), m_selections.end(), entity);
        if(it != m_selections.end() && *it == entity) {
            return;
        }
        m_selections.insert(it, entity);
        ++m_selectionVersion;
    }
    entitySelected.emit(entity);
}

void SelectionState::removeSelection(const Core::EntityRef& entity) {
    {
        std::unique_lock lock(m_mutex);
        const auto it = std::lower_bound(m_selections.begin(), m_selections.end(), entity);
        if(it == m_selections.end() || *it != entity) {
            return;
        }
        m_selections.erase(it);
        ++m_selectionVersion;
    }
    entityDeselected.emit(entity);
}

void SelectionState::clearSelection() {
    {
        std::unique_lock lock(m_mutex);
        if(m_selections.empty()) {
            return;
        }
        m_selections.clear();
        ++m_selectionVersion;
    }
    selectionCleared.emit();
}

std::vector<Core::EntityRef> SelectionState::selections() const {
    std::shared_lock lock(m_mutex);
    return m_selections;
}

bool SelectionState::isSelected(const Core::EntityRef& entity) const {
    std::shared_lock lock(m_mutex);
    return std::binary_search(m_selections.begin(), m_selections.end(), entity);
}

void SelectionState::setHovered(const Core::EntityRef& entity) {
    if(!entity.isValid()) {
        clearHover();
        return;
    }
    {
        std::unique_lock lock(m_mutex);
        if(m_hovered.has_value() && *m_hovered == entity) {
            return;
        }
        m_hovered = entity;
        ++m_hoverVersion;
    }
    hoverChanged.emit(entity);
}

void SelectionState::clearHover() {
    {
        std::unique_lock lock(m_mutex);
        if(!m_hovered.has_value()) {
            return;
        }
        m_hovered.reset();
        ++m_hoverVersion;
    }
    hoverChanged.emit(std::nullopt);
}

std::optional<Core::EntityRef> SelectionState::hovered() const {
    std::shared_lock lock(m_mutex);
    return m_hovered;
}

uint64_t SelectionState::selectionVersion() const noexcept {
    return m_selectionVersion.load(std::memory_order_acquire);
}

uint64_t SelectionState::hoverVersion() const noexcept {
    return m_hoverVersion.load(std::memory_order_acquire);
}

} // namespace OpenGeoLab::Scene
