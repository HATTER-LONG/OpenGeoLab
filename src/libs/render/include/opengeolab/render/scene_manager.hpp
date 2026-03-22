/**
 * @file scene_manager.hpp
 * @brief Manages the Coin3D scene graph lifecycle.
 */

#pragma once

#include <opengeolab/render/camera_state.hpp>
#include <opengeolab/render/render_export.hpp>

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <string>
#include <string_view>

class SoSeparator;
class SoCamera;

namespace OpenGeoLab::Render {

/// Rendering display modes for 3D objects.
enum class DisplayMode { kFlatLines, kWireframe };

/**
 * @brief Manages Coin3D scene graph: root tree, camera, light, geometry nodes.
 *
 * Responsibilities: SoDB init, scene tree construction, camera/light management,
 * geometry node add/remove. Does NOT drive rendering (CoinQuickItem does that)
 * or handle events (NavigationController does that).
 */
class OPENGEOLAB_RENDER_EXPORT SceneManager {
public:
    SceneManager();
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    auto operator=(const SceneManager&) -> SceneManager& = delete;
    SceneManager(SceneManager&&) noexcept;
    auto operator=(SceneManager&&) noexcept -> SceneManager&;

    /// Initialize Coin3D and build the scene tree. Must be called before rendering.
    void initialize();

    /// @return Root scene separator for GL rendering.
    [[nodiscard]] auto root_node() const -> SoSeparator*;

    /// @return Current active camera node.
    [[nodiscard]] auto camera() const -> SoCamera*;

    /**
     * @brief Add a box geometry using SoIndexedFaceSet.
     * @param size_x Box extent along X
     * @param size_y Box extent along Y
     * @param size_z Box extent along Z
     * @return Unique node ID string
     */
    auto add_box(float size_x, float size_y, float size_z) -> std::string;

    /// Remove a geometry node by its ID.
    void remove_node(std::string_view node_id);

    /// Set the display mode for all objects.
    void set_display_mode(DisplayMode mode);

    /// Get the current display mode.
    [[nodiscard]] auto display_mode() const -> DisplayMode;

    /// @return Snapshot of current camera parameters.
    [[nodiscard]] auto camera_state() const -> CameraState;

    /// Restore camera from a state snapshot.
    void restore_camera_state(const CameraState& state);

    /// Adjust camera to fit all objects in the viewport.
    void view_all(int viewport_width, int viewport_height);

    /// @return JSON description of all scene nodes.
    [[nodiscard]] auto describe_scene() const -> nlohmann::json;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace OpenGeoLab::Render
