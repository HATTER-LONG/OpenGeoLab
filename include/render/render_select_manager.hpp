/**
 * @file render_select_manager.hpp
 * @brief Centralized selection and hover state management for the render layer.
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_types.hpp"
#include "util/signal.hpp"

#include <cstdint>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief Result of a single GPU pick operation, identifying an entity by type and UID.
 */
struct PickResult {
    uint64_t m_uid{0}; ///< Type-scoped unique identifier, Note: MeshNode only has node id
    RenderEntityType m_type{RenderEntityType::None}; ///< Entity type for selection filtering

    bool operator==(const PickResult& other) const {
        return m_uid == other.m_uid && m_type == other.m_type;
    }
};

/** @brief Hash functor for PickResult, suitable for unordered containers. */
struct PickResultHash {
    static constexpr void hashCombine(size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }
    std::size_t operator()(const PickResult& result) const noexcept {
        std::size_t seed = 0;
        hashCombine(seed, result.m_uid);
        hashCombine(seed, static_cast<std::underlying_type_t<RenderEntityType>>(result.m_type));
        return seed;
    }
};

using PickResultSet = std::unordered_set<PickResult, PickResultHash>;

/**
 * @brief Describes how the selection set was mutated.
 */
enum class SelectionChangeAction : uint8_t {
    Added = 0,   ///< Entity was added to the selection
    Removed = 1, ///< Entity was removed from the selection
    Cleared = 2  ///< Entire selection was cleared
};

/**
 * @brief Singleton managing pick-selection and hover state across the render layer.
 *
 * Thread-safe: all public methods lock m_mutex. The render thread updates hover
 * state while the GUI thread reads selections and subscribes to change signals.
 */
class RenderSelectManager : public Kangaroo::Util::NonCopyMoveable {
private:
    RenderSelectManager() = default;

public:
    static RenderSelectManager& instance();
    ~RenderSelectManager() = default;

    /** @brief Enable or disable the pick system globally. */
    void setPickEnabled(bool enabled);
    /** @brief Check whether picking is currently enabled. */
    [[nodiscard]] bool isPickEnabled() const;

    /**
     * @brief Set which entity types are eligible for picking.
     * @param types Bitmask of pickable entity types.
     */
    void setPickTypes(RenderEntityTypeMask types);

    /** @brief Current pick-type bitmask. */
    [[nodiscard]] RenderEntityTypeMask getPickTypes() const;

    /** @brief Check whether a specific entity type is currently pickable. */
    bool isTypePickable(RenderEntityType type) const;

    /**
     * @brief Normalize a pick-type mask by resolving implicit type hierarchies.
     * @param types Raw bitmask from the user.
     * @return Expanded mask with dependent types included.
     */
    RenderEntityTypeMask normalizePickTypes(RenderEntityTypeMask types);

    /**
     * @brief Add an entity to the current selection.
     * @return True if the entity was newly added (false if already selected).
     */
    bool addSelection(uint64_t entity_uid, RenderEntityType type);

    /**
     * @brief Remove an entity from the current selection.
     * @return True if the entity was found and removed.
     */
    bool removeSelection(uint64_t entity_uid, RenderEntityType type);

    /** @brief Clear the entire selection set and emit Cleared signal. */
    void clearSelection();

    /** @brief Snapshot of all currently selected entities. */
    [[nodiscard]] std::vector<PickResult> selections() const;

    // ── Hover state ─────────────────────────────────────────────────────
    /**
     * @brief Set the currently hovered entity and its ownership context.
     * @param uid        Entity UID.
     * @param type       Entity type.
     * @param partUid    Parent part UID (for part-level hover highlight).
     * @param wireUid    Parent wire UID (for edge-to-wire hover resolution).
     */
    void setHoverEntity(uint64_t uid,
                        RenderEntityType type,
                        uint64_t part_uid = 0,
                        uint64_t wire_uid = 0);
    /** @brief Clear the hovered entity. */
    void clearHover();
    /** @brief Get the currently hovered entity (type + uid). */
    [[nodiscard]] PickResult hoveredEntity() const;
    /** @brief Check whether a specific entity (by key) is currently hovered. */
    [[nodiscard]] bool isEntityHovered(const RenderNodeKey& key) const;
    /** @brief Check whether a part is currently hovered. */
    [[nodiscard]] bool isPartHovered(uint64_t part_uid) const;
    /** @brief Check whether a wire is currently hovered. */
    [[nodiscard]] bool isWireHovered(uint64_t wire_uid) const;

    /**
     * @brief Set the edge UIDs belonging to the currently hovered wire.
     * @param edge_uids All edge UIDs that form the hovered wire's closed loop.
     */
    void setHoveredWireEdges(const std::vector<uint64_t>& edge_uids);

    /** @brief Check whether an edge belongs to the currently hovered wire. */
    [[nodiscard]] bool isEdgeInHoveredWire(uint64_t edge_uid) const;

    // ── Selection query ─────────────────────────────────────────────────

    /** @brief Check whether a specific entity (by key) is in the selection set. */
    [[nodiscard]] bool isSelected(const RenderNodeKey& key) const;
    /** @brief Check whether a part is in the selection set. */
    [[nodiscard]] bool isPartSelected(uint64_t part_uid) const;
    /** @brief Check whether a wire is in the selection set. */
    [[nodiscard]] bool isWireSelected(uint64_t wire_uid) const;

    /**
     * @brief Record all edge UIDs belonging to a selected wire.
     * @param wire_uid The wire being selected.
     * @param edge_uids All edge UIDs forming the wire's closed loop.
     */
    void addSelectedWireEdges(uint64_t wire_uid, const std::vector<uint64_t>& edge_uids);

    /** @brief Remove edge UIDs associated with a deselected wire. */
    void removeSelectedWireEdges(uint64_t wire_uid);

    /** @brief Clear all wire-edge associations for selected wires. */
    void clearSelectedWireEdges();

    /** @brief Check whether an edge belongs to any currently selected wire. */
    [[nodiscard]] bool isEdgeInSelectedWire(uint64_t edge_uid) const;

    Util::ScopedConnection subscribePickEnabledChanged(std::function<void(bool)> callback) {
        return m_pickEnabledChanged.connect(std::move(callback));
    }

    Util::ScopedConnection
    subscribePickSettingsChanged(std::function<void(RenderEntityTypeMask)> callback) {
        return m_pickSettingsChanged.connect(std::move(callback));
    }

    Util::ScopedConnection
    subscribeSelectionChanged(std::function<void(PickResult, SelectionChangeAction)> callback) {
        return m_selectionChanged.connect(std::move(callback));
    }

    Util::ScopedConnection subscribeHoverChanged(std::function<void()> callback) {
        return m_hoverChanged.connect(std::move(callback));
    }

private:
    mutable std::mutex m_mutex; ///< Guards all selection/hover state (render + GUI threads)
    bool m_pickEnabled{false};  ///< Global pick enable flag
    RenderEntityTypeMask m_pickTypes{RenderEntityTypeMask::None}; ///< Active pick-type bitmask

    PickResultSet m_currentSelections; ///< Set of currently selected entities

    PickResult m_hoveredEntity{0, RenderEntityType::None}; ///< Entity under the cursor
    uint64_t m_hoveredPartUid{0};                          ///< Parent part uid of hovered entity
    uint64_t m_hoveredWireUid{0};                          ///< Parent wire uid of hovered edge
    std::unordered_set<uint64_t> m_hoveredWireEdgeUids;    ///< All edge UIDs in hovered wire

    /// Per-selected-wire edge UID sets for complete wire highlighting
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> m_selectedWireEdges;

    Util::Signal<RenderEntityTypeMask> m_pickSettingsChanged; ///< Fired when pickable types change
    Util::Signal<bool> m_pickEnabledChanged; ///< Fired when pick enabled state changes
    Util::Signal<PickResult, SelectionChangeAction>
        m_selectionChanged;        ///< Fired on selection mutations
    Util::Signal<> m_hoverChanged; ///< Fired when hovered entity changes
};
} // namespace OpenGeoLab::Render