/**
 * @file select_manager.hpp
 * @brief Global selection state manager for interactive picking
 *
 * SelectManager is a render-layer singleton that stores:
 * - Whether the viewport is currently in picking mode
 * - Which entity types are eligible for picking
 * - The current set of picked results (RenderEntityType + uid56)
 *
 * Uses RenderEntityType to unify geometry and mesh entity identification.
 */

#pragma once
#include "render/render_types.hpp"
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
     * @brief A picked entity reference using RenderEntityType + 56-bit uid
     */
    struct PickResult {
        RenderEntityType m_type{RenderEntityType::None};
        uint64_t m_uid56{0};

        PickResult() = default;
        PickResult(RenderEntityType type, uint64_t uid56) : m_type(type), m_uid56(uid56) {}

        /// Construct from RenderUID
        explicit PickResult(RenderUID uid) : m_type(uid.type()), m_uid56(uid.uid56()) {}

        /// Convert to RenderUID
        [[nodiscard]] RenderUID toRenderUID() const noexcept {
            return RenderUID::encode(m_type, m_uid56);
        }

        bool operator==(const PickResult& other) const noexcept {
            return m_type == other.m_type && m_uid56 == other.m_uid56;
        }
        [[nodiscard]] bool isValid() const noexcept {
            return (m_type != RenderEntityType::None) && (m_uid56 != 0);
        }

        struct Hash {
            std::size_t operator()(const PickResult& pr) const noexcept {
                std::size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t>(pr.m_type));
                std::size_t h2 = std::hash<uint64_t>{}(pr.m_uid56);
                return h1 ^ (h2 << 1);
            }
        };
    };

    /**
     * @brief Action describing how a selection changed
     */
    enum class SelectionChangeAction : uint8_t { Added = 0, Removed = 1, Cleared = 2 };

public:
    static SelectManager& instance();

    SelectManager();
    ~SelectManager();

    /** @brief Enable or disable interactive picking mode. */
    void setPickEnabled(bool enabled);
    /** @brief Whether interactive picking mode is currently active. */
    [[nodiscard]] bool isPickEnabled() const;

    /** @brief Set the bitmask of entity types eligible for picking. */
    void setPickTypes(PickTypes types);
    /** @brief Current bitmask of pickable entity types. */
    [[nodiscard]] PickTypes pickTypes() const;

    /**
     * @brief Check whether entities of @p type are currently pickable.
     * @param type The render entity type to query.
     */
    [[nodiscard]] bool isTypePickable(RenderEntityType type) const;

    /**
     * @brief Add an entity to the selection set.
     * @return true if the entity was newly added, false if already selected.
     */
    bool addSelection(uint64_t uid56, RenderEntityType type);
    bool addSelection(const PickResult& pr) { return addSelection(pr.m_uid56, pr.m_type); }

    /**
     * @brief Remove an entity from the selection set.
     * @return true if the entity was removed, false if not found.
     */
    bool removeSelection(uint64_t uid56, RenderEntityType type);
    bool removeSelection(const PickResult& pr) { return removeSelection(pr.m_uid56, pr.m_type); }

    /** @brief Check whether an entity is in the current selection set. */
    [[nodiscard]] bool containsSelection(uint64_t uid56, RenderEntityType type) const;
    [[nodiscard]] bool containsSelection(const PickResult& pr) const {
        return containsSelection(pr.m_uid56, pr.m_type);
    }

    /** @brief Return a snapshot of all currently selected entities. */
    [[nodiscard]] std::vector<PickResult> selections() const;

    /** @brief Remove all entities from the selection set. */
    void clearSelections();

    /**
     * @brief Subscribe to pick type changes.
     * @param callback Invoked with the new PickTypes bitmask.
     * @return Scoped connection that disconnects on destruction.
     */
    [[nodiscard]] Util::ScopedConnection
    subscribePickSettingsChanged(std::function<void(PickTypes)> callback);

    /**
     * @brief Subscribe to pick enabled/disabled state changes.
     * @param callback Invoked with the new enabled state.
     * @return Scoped connection that disconnects on destruction.
     */
    [[nodiscard]] Util::ScopedConnection
    subscribePickEnabledChanged(std::function<void(bool)> callback);

    /**
     * @brief Subscribe to selection changes (add, remove, clear).
     * @param callback Invoked with the affected entity and the action performed.
     * @return Scoped connection that disconnects on destruction.
     */
    [[nodiscard]] Util::ScopedConnection
    subscribeSelectionChanged(std::function<void(PickResult, SelectionChangeAction)> callback);

private:
    [[nodiscard]] static PickTypes normalizePickTypes(PickTypes types);
    [[nodiscard]] static bool isValidSelection(uint64_t uid56, RenderEntityType type);

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
