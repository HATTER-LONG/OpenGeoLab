/**
 * @file select_manager.hpp
 * @brief Global selection state manager for interactive picking
 *
 * SelectManager is a render-layer singleton that stores:
 * - Whether the viewport is currently in picking mode
 * - Which entity types are eligible for picking
 * - The current set of picked results (type + uid)
 *
 * It exposes lightweight signals (Util::Signal) so UI/viewports can subscribe
 * and trigger redraws when selection state changes.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "util/signal.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Selection manager singleton for the render module
 */
class SelectManager {
public:
    /**
     * @brief Bitmask of selectable entity categories
     *
     * - Vertex/Edge/Face can be combined.
     * - Solid/Part are mutually exclusive and also exclusive with Vertex/Edge/Face.
     */
    enum class PickTypes : uint8_t {
        None = 0,
        Vertex = 1u << 0u,
        Edge = 1u << 1u,
        Face = 1u << 2u,
        Solid = 1u << 3u,
        Part = 1u << 4u,
    };

    /**
     * @brief A picked entity reference
     */
    struct PickResult {
        Geometry::EntityType m_type{Geometry::EntityType::None};
        Geometry::EntityUID m_uid{Geometry::INVALID_ENTITY_UID};

        [[nodiscard]] bool isValid() const {
            return (m_type != Geometry::EntityType::None) &&
                   (m_uid != Geometry::INVALID_ENTITY_UID);
        }
    };

public:
    /**
     * @brief Get singleton instance
     */
    static SelectManager& instance();

    SelectManager(const SelectManager&) = delete;
    SelectManager& operator=(const SelectManager&) = delete;
    SelectManager(SelectManager&&) = delete;
    SelectManager& operator=(SelectManager&&) = delete;

    SelectManager();
    ~SelectManager();

    /**
     * @brief Enable/disable picking mode
     */
    void setPickEnabled(bool enabled);

    /**
     * @brief Check whether picking mode is enabled
     */
    [[nodiscard]] bool isPickEnabled() const;

    /**
     * @brief Set allowed pick types (will be normalized to satisfy exclusivity rules)
     */
    void setPickTypes(PickTypes types);

    /**
     * @brief Get allowed pick types
     */
    [[nodiscard]] PickTypes pickTypes() const;

    /**
     * @brief Check if the given entity type is allowed by the current pick types
     */
    [[nodiscard]] bool isTypePickable(Geometry::EntityType type) const;

    /**
     * @brief Add a pick result to the selection set
     * @return true if added (was not present)
     */
    bool addSelection(Geometry::EntityType type, Geometry::EntityUID uid);

    /**
     * @brief Remove a pick result from the selection set
     * @return true if removed (was present)
     */
    bool removeSelection(Geometry::EntityType type, Geometry::EntityUID uid);

    /**
     * @brief Check if a pick result exists in the current selection set
     */
    [[nodiscard]] bool containsSelection(Geometry::EntityType type, Geometry::EntityUID uid) const;

    /**
     * @brief Get current selection results (copy)
     */
    [[nodiscard]] std::vector<PickResult> selections() const;

    /**
     * @brief Clear all selection results
     */
    void clearSelections();

    /**
     * @brief Subscribe to changes in pick enable/types
     */
    [[nodiscard]] Util::ScopedConnection
    subscribePickSettingsChanged(std::function<void()> callback);

    /**
     * @brief Subscribe to changes in selection results
     */
    [[nodiscard]] Util::ScopedConnection subscribeSelectionChanged(std::function<void()> callback);

private:
    [[nodiscard]] static PickTypes normalizePickTypes(PickTypes types);
    [[nodiscard]] static bool isValidSelection(Geometry::EntityType type, Geometry::EntityUID uid);

    [[nodiscard]] static uint64_t makeKey(Geometry::EntityType type, Geometry::EntityUID uid);

private:
    mutable std::mutex m_mutex;

    bool m_pickEnabled{false};
    PickTypes m_pickTypes{PickTypes::Face};

    std::vector<PickResult> m_selections;

    Util::Signal<> m_pickSettingsChanged;
    Util::Signal<> m_selectionChanged;
};

// -----------------------------------------------------------------------------
// Bitmask helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr inline SelectManager::PickTypes operator|(SelectManager::PickTypes a,
                                                                  SelectManager::PickTypes b) {
    return static_cast<SelectManager::PickTypes>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

[[nodiscard]] constexpr inline SelectManager::PickTypes operator&(SelectManager::PickTypes a,
                                                                  SelectManager::PickTypes b) {
    return static_cast<SelectManager::PickTypes>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

constexpr inline SelectManager::PickTypes& operator|=(SelectManager::PickTypes& a,
                                                      SelectManager::PickTypes b) {
    a = a | b;
    return a;
}

} // namespace OpenGeoLab::Render
