/**
 * @file label_manager.hpp
 * @brief LabelManager — 3D annotation label storage
 *
 * Phase 1 provides data-only label storage. Phase 2 adds MSDF rendering
 * via LabelPass. Independent of SelectionState so any tool can add labels.
 */

#pragma once

#include <opengeolab/core/entity_ref.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <glm/vec4.hpp>
#include <kangaroo/util/signal.hpp>

#include <atomic>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <vector>

namespace OpenGeoLab::Scene {

/// A 3D annotation label attached to an entity.
struct Label3D {
    Core::EntityRef entity;                      ///< Which entity to attach to.
    std::string text;                            ///< Display text ("F:3", "V:1").
    glm::vec4 textColor{1.0F, 1.0F, 1.0F, 1.0F}; ///< White text by default.
    glm::vec4 bgColor{0.0F, 0.0F, 0.0F, 0.7F};   ///< Semi-transparent black.
};

/**
 * @brief Thread-safe label storage with version-based dirty tracking.
 *
 * LabelPass (Phase 2) reads labels and computes 3D anchor positions
 * from the entity's geometry data in GpuBufferManager.
 */
class OPENGEOLAB_SCENE_EXPORT LabelManager final {
public:
    LabelManager();
    ~LabelManager();

    /// Add or replace a label for an entity.
    void addLabel(Label3D label);

    /// Remove any label associated with an entity.
    void removeByEntity(const Core::EntityRef& entity);

    /// Remove all labels.
    void clearLabels();

    /// Snapshot current labels for thread-safe readers.
    [[nodiscard]] std::vector<Label3D> labels() const;

    /// Version incremented whenever label contents change.
    [[nodiscard]] uint64_t version() const noexcept;

    Kangaroo::Util::Signal<> labelsChanged;    ///< Emitted after label storage changes.
    Kangaroo::Util::Signal<> visibleChanged;   ///< Emitted when visibility changes.
    Kangaroo::Util::Signal<> autoLabelChanged; ///< Emitted when auto-label changes.

    /// Set whether labels should be rendered. Default: false.
    void setVisible(bool visible);

    /// Whether labels should be rendered.
    [[nodiscard]] bool isVisible() const noexcept;

    /// Set whether auto-label on selection is enabled. Default: false.
    void setAutoLabel(bool enabled);

    /// Whether auto-label on selection is enabled.
    [[nodiscard]] bool autoLabel() const noexcept;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<Label3D> m_labels;
    std::atomic<uint64_t> m_version{0};
    std::atomic<bool> m_visible{false};
    std::atomic<bool> m_autoLabel{false};
};

} // namespace OpenGeoLab::Scene
