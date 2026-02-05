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
#include <kangaroo/util/noncopyable.hpp>

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Selection manager singleton for the render module
 */
class SelectManager : private Kangaroo::Util::NonCopyMoveable {
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
        bool operator==(const PickResult& other) const noexcept {
            return m_type == other.m_type && m_uid == other.m_uid;
        }
        [[nodiscard]] bool isValid() const noexcept {
            return (m_type != Geometry::EntityType::None) &&
                   (m_uid != Geometry::INVALID_ENTITY_UID);
        }

        struct Hash {
            std::size_t operator()(const PickResult& pr) const noexcept {
                std::size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t>(pr.m_type));
                std::size_t h2 = std::hash<Geometry::EntityUID>{}(pr.m_uid);
                return h1 ^ (h2 << 1);
            }
        };
    };

public:
    /**
     * @brief Get singleton instance
     */
    static SelectManager& instance();

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
    bool addSelection(Geometry::EntityUID uid, Geometry::EntityType type);

    /**
     * @brief Remove a pick result from the selection set
     * @return true if removed (was present)
     */
    bool removeSelection(Geometry::EntityUID uid, Geometry::EntityType type);

    /**
     * @brief Check if a pick result exists in the current selection set
     */
    [[nodiscard]] bool containsSelection(Geometry::EntityUID uid, Geometry::EntityType type) const;

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
    [[nodiscard]] static bool isValidSelection(Geometry::EntityUID uid, Geometry::EntityType type);

private:
    mutable std::mutex m_mutex;

    bool m_pickEnabled{true};
    PickTypes m_pickTypes{PickTypes::Face};

    std::unordered_set<PickResult, PickResult::Hash> m_selections;

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
