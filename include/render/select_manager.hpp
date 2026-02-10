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
     * @brief Pick type bitmask for filtering selectable entity types
     */
    enum class PickTypes : uint32_t {
        None = 0,
        Vertex = 1u << 0u,
        Edge = 1u << 1u,
        Wire = 1u << 2u,
        Face = 1u << 3u,
        Solid = 1u << 4u,
        Part = 1u << 5u,
        MeshNode = 1u << 6u,
        MeshElement = 1u << 7u,
    };

    /**
     * @brief A picked entity reference
     */
    struct PickResult {
        Geometry::EntityType m_type{Geometry::EntityType::None};
        Geometry::EntityUID m_uid{Geometry::INVALID_ENTITY_UID};

        PickResult() = default;
        PickResult(Geometry::EntityType type, Geometry::EntityUID uid) : m_type(type), m_uid(uid) {}

        /// Construct from EntityRef for seamless interop.
        PickResult(const Geometry::EntityRef& ref)
            : m_type(ref.m_type), m_uid(ref.m_uid) {} // NOLINT

        /// Convert to EntityRef (lightweight, no allocation).
        [[nodiscard]] operator Geometry::EntityRef() const noexcept { // NOLINT
            return {m_uid, m_type};
        }

        /// Convert to EntityRef explicitly.
        [[nodiscard]] Geometry::EntityRef toRef() const noexcept { return {m_uid, m_type}; }

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

    /**
     * @brief Action describing how a selection changed
     */
    enum class SelectionChangeAction : uint8_t { Added = 0, Removed = 1, Cleared = 2 };

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
    /// @overload EntityRef-based convenience.
    bool addSelection(const Geometry::EntityRef& ref) {
        return addSelection(ref.m_uid, ref.m_type);
    }

    /**
     * @brief Remove a pick result from the selection set
     * @return true if removed (was present)
     */
    bool removeSelection(Geometry::EntityUID uid, Geometry::EntityType type);
    /// @overload EntityRef-based convenience.
    bool removeSelection(const Geometry::EntityRef& ref) {
        return removeSelection(ref.m_uid, ref.m_type);
    }

    /**
     * @brief Check if a pick result exists in the current selection set
     */
    [[nodiscard]] bool containsSelection(Geometry::EntityUID uid, Geometry::EntityType type) const;
    /// @overload EntityRef-based convenience.
    [[nodiscard]] bool containsSelection(const Geometry::EntityRef& ref) const {
        return containsSelection(ref.m_uid, ref.m_type);
    }

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
    subscribePickSettingsChanged(std::function<void(PickTypes)> callback);

    /**
     * @brief Subscribe to changes in pick enabled state
     */
    [[nodiscard]] Util::ScopedConnection
    subscribePickEnabledChanged(std::function<void(bool)> callback);

    /**
     * @brief Subscribe to changes in selection results
     */
    [[nodiscard]] Util::ScopedConnection
    subscribeSelectionChanged(std::function<void(PickResult, SelectionChangeAction)> callback);

private:
    [[nodiscard]] static PickTypes normalizePickTypes(PickTypes types);
    [[nodiscard]] static bool isValidSelection(Geometry::EntityUID uid, Geometry::EntityType type);

private:
    mutable std::mutex m_mutex;

    bool m_pickEnabled{false};
    PickTypes m_pickTypes{PickTypes::None};

    std::unordered_set<PickResult, PickResult::Hash> m_selections;

    Util::Signal<PickTypes> m_pickSettingsChanged;
    Util::Signal<bool> m_pickEnabledChanged;
    Util::Signal<PickResult, SelectionChangeAction> m_selectionChanged;
};

// -----------------------------------------------------------------------------
// Bitmask helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr inline SelectManager::PickTypes operator|(SelectManager::PickTypes a,
                                                                  SelectManager::PickTypes b) {
    return static_cast<SelectManager::PickTypes>(static_cast<uint32_t>(a) |
                                                 static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr inline SelectManager::PickTypes operator&(SelectManager::PickTypes a,
                                                                  SelectManager::PickTypes b) {
    return static_cast<SelectManager::PickTypes>(static_cast<uint32_t>(a) &
                                                 static_cast<uint32_t>(b));
}

constexpr inline SelectManager::PickTypes& operator|=(SelectManager::PickTypes& a,
                                                      SelectManager::PickTypes b) {
    a = a | b;
    return a;
}

} // namespace OpenGeoLab::Render
