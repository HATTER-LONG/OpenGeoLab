/**
 * @file selection_state.hpp
 * @brief SelectionState — thread-safe entity-level selection manager
 *
 * Manages the set of selected and hovered entities for 3D picking.
 * Coexists with SceneGraph's node-level selection (used for tree-view).
 * Uses version-based dirty tracking so the render thread only re-resolves
 * EntityRef → DrawRange when selection actually changes.
 */

#pragma once

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/core/pick_mask.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <kangaroo/util/signal.hpp>

#include <atomic>
#include <optional>
#include <shared_mutex>
#include <vector>

namespace OpenGeoLab::Scene {

/**
 * @brief Thread-safe entity-level selection state.
 *
 * Writers (main thread: pick dispatch, Python actions, QML commands)
 * increment version atomically. Readers (render synchronize, QML reads)
 * use the version to detect changes without locking during render.
 */
class OPENGEOLAB_SCENE_EXPORT SelectionState final {
public:
    SelectionState();
    ~SelectionState();

    void setPickEnabled(bool enabled);
    [[nodiscard]] bool pickEnabled() const;

    void setPickMask(Core::PickMask mask);
    [[nodiscard]] Core::PickMask pickMask() const;

    void addSelection(const Core::EntityRef& entity);
    void removeSelection(const Core::EntityRef& entity);
    void clearSelection();
    [[nodiscard]] std::vector<Core::EntityRef> selections() const;
    [[nodiscard]] bool isSelected(const Core::EntityRef& entity) const;

    void setHovered(const Core::EntityRef& entity);
    void clearHover();
    [[nodiscard]] std::optional<Core::EntityRef> hovered() const;

    [[nodiscard]] uint64_t selectionVersion() const noexcept;
    [[nodiscard]] uint64_t hoverVersion() const noexcept;

    Kangaroo::Util::Signal<Core::EntityRef> entitySelected;
    Kangaroo::Util::Signal<Core::EntityRef> entityDeselected;
    Kangaroo::Util::Signal<> selectionCleared;
    Kangaroo::Util::Signal<Core::EntityRef> hoverChanged;
    Kangaroo::Util::Signal<> pickConfigChanged;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<Core::EntityRef> m_selections; ///< Sorted for O(log n) lookup
    std::optional<Core::EntityRef> m_hovered;
    Core::PickMask m_pickMask{Core::PickMask::None};
    bool m_pickEnabled{false};
    std::atomic<uint64_t> m_selectionVersion{0};
    std::atomic<uint64_t> m_hoverVersion{0};
};

} // namespace OpenGeoLab::Scene
