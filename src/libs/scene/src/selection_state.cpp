/**
 * @file selection_state.cpp
 * @brief SelectionState implementation
 */

#include <opengeolab/scene/selection_state.hpp>

#include <algorithm>
#include <span>

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
    addSelections(std::span<const Core::EntityRef>(&entity, 1));
}

void SelectionState::removeSelection(const Core::EntityRef& entity) {
    removeSelections(std::span<const Core::EntityRef>(&entity, 1));
}

void SelectionState::addSelections(std::initializer_list<Core::EntityRef> entities) {
    addSelections(std::span<const Core::EntityRef>(entities.begin(), entities.size()));
}

void SelectionState::removeSelections(std::initializer_list<Core::EntityRef> entities) {
    removeSelections(std::span<const Core::EntityRef>(entities.begin(), entities.size()));
}

void SelectionState::addSelections(std::span<const Core::EntityRef> entities) {
    std::vector<Core::EntityRef> sorted;
    sorted.reserve(entities.size());
    for(const auto& entity : entities) {
        if(entity.isValid()) {
            sorted.push_back(entity);
        }
    }
    if(sorted.empty()) {
        return;
    }
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    std::vector<Core::EntityRef> added;
    {
        std::unique_lock lock(m_mutex);
        added.reserve(sorted.size());
        std::vector<Core::EntityRef> merged;
        merged.reserve(m_selections.size() + sorted.size());

        auto selectionIt = m_selections.begin();
        auto newIt = sorted.begin();
        while(selectionIt != m_selections.end() && newIt != sorted.end()) {
            if(*selectionIt < *newIt) {
                merged.push_back(*selectionIt);
                ++selectionIt;
            } else if(*newIt < *selectionIt) {
                added.push_back(*newIt);
                merged.push_back(*newIt);
                ++newIt;
            } else {
                merged.push_back(*selectionIt);
                ++selectionIt;
                ++newIt;
            }
        }
        while(selectionIt != m_selections.end()) {
            merged.push_back(*selectionIt);
            ++selectionIt;
        }
        while(newIt != sorted.end()) {
            added.push_back(*newIt);
            merged.push_back(*newIt);
            ++newIt;
        }

        if(added.empty()) {
            return;
        }
        m_selections = std::move(merged);
        ++m_selectionVersion;
    }
    entitiesSelected.emit(std::move(added));
}

void SelectionState::removeSelections(std::span<const Core::EntityRef> entities) {
    if(entities.empty()) {
        return;
    }

    std::vector<Core::EntityRef> sorted(entities.begin(), entities.end());
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    std::vector<Core::EntityRef> removed;
    {
        std::unique_lock lock(m_mutex);
        removed.reserve(sorted.size());
        std::vector<Core::EntityRef> remaining;
        remaining.reserve(m_selections.size());

        auto selectionIt = m_selections.begin();
        auto removeIt = sorted.begin();
        while(selectionIt != m_selections.end() && removeIt != sorted.end()) {
            if(*selectionIt < *removeIt) {
                remaining.push_back(*selectionIt);
                ++selectionIt;
            } else if(*removeIt < *selectionIt) {
                ++removeIt;
            } else {
                removed.push_back(*selectionIt);
                ++selectionIt;
                ++removeIt;
            }
        }
        while(selectionIt != m_selections.end()) {
            remaining.push_back(*selectionIt);
            ++selectionIt;
        }

        if(removed.empty()) {
            return;
        }
        m_selections = std::move(remaining);
        ++m_selectionVersion;
    }
    entitiesDeselected.emit(std::move(removed));
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
