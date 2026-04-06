/**
 * @file frame_state.hpp
 * @brief Per-frame rendering state passed to each render pass
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/label_colors.hpp>
#include <opengeolab/core/pick_mask.hpp>
#include <opengeolab/scene/display_mode.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace OpenGeoLab::Render {

/// A draw range annotated with entity type for style-differentiated highlighting.
struct HighlightEntry {
    Scene::DrawRange range;
    Core::EntityType entityType{Core::EntityType::GeoFace};
};

/// A label resolved to a 3D world-space anchor, ready for LabelPass rendering.
struct ResolvedLabel {
    glm::vec3 anchorWorld{};     ///< 3D world-space anchor point
    std::string text;            ///< Display text ("F:3", "V:1")
    glm::vec4 textColor{};      ///< Entity-type color
    glm::vec4 bgColor{};        ///< Background color (with alpha)
    Core::EntityType entityType{Core::EntityType::GeoFace};
    uint32_t stackIndex{0};     ///< Vertical offset for overlapping anchors
    bool occluded{false};        ///< True if anchor is behind geometry
};

/** @brief Immutable per-frame state consumed by every render pass. */
struct FrameState {
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projMatrix{1.0f};
    glm::vec3 cameraPos{0.0f};
    float devicePixelRatio{1.0f};

    int viewportWidth{0};
    int viewportHeight{0};

    bool xRayMode{false};
    bool showTessellation{false};  ///< Overlay tessellation triangle edges and vertices.
    Scene::DisplayModeMask displayMask{Scene::DisplayModeMask::Surface |
                                       Scene::DisplayModeMask::Wireframe};

    std::vector<HighlightEntry> selectedEntries;
    std::vector<HighlightEntry> hoveredEntries;

    /// Pick mask passed to SelectionPass for GPU-level entity filtering.
    Core::PickMask activePickMask{Core::PickMask::All};

    /// Phase 2: Resolved labels for LabelPass rendering.
    std::vector<ResolvedLabel> resolvedLabels;

    /// Whether labels should be rendered.
    bool labelsVisible{false};
};

} // namespace OpenGeoLab::Render
