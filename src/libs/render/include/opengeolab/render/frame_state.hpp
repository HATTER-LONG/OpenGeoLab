/**
 * @file frame_state.hpp
 * @brief Per-frame rendering state passed to each render pass
 */

#pragma once

#include <opengeolab/scene/display_mode.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <glm/glm.hpp>

#include <vector>

namespace OpenGeoLab::Render {

struct FrameState {
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projMatrix{1.0f};
    glm::vec3 cameraPos{0.0f};
    float devicePixelRatio{1.0f};

    int viewportWidth{0};
    int viewportHeight{0};

    bool xRayMode{false};
    Scene::DisplayModeMask displayMask{
        Scene::DisplayModeMask::Surface | Scene::DisplayModeMask::Wireframe};

    std::vector<Scene::DrawRange> selectedDrawRanges;
    std::vector<Scene::DrawRange> hoveredDrawRanges;
};

} // namespace OpenGeoLab::Render
