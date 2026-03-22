/**
 * @file navigation_controller.hpp
 * @brief Stateless navigation calculator for pan and zoom.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <array>

namespace OpenGeoLab::Render {

/**
 * @brief Stateless pan and zoom calculator.
 *
 * Receives mouse deltas and current camera parameters, returns new camera
 * parameters. Does not depend on Coin3D types — uses std::array<float, N>
 * for all spatial data. Designed for independent unit testing.
 */
class OPENGEOLAB_RENDER_EXPORT NavigationController {
public:
    /// Result of a pan operation.
    struct PanResult {
        std::array<float, 3> new_position;
    };

    /// Result of a zoom operation.
    struct ZoomResult {
        float new_focal_distance;
        std::array<float, 3> new_position; ///< Perspective: moved along view direction
        float new_height;                  ///< Orthographic: scaled viewport height
    };

    /**
     * @brief Compute pan translation along camera view plane.
     * @param current_position Current camera world position
     * @param current_orientation Current camera orientation quaternion
     * @param dx Normalized horizontal mouse delta
     * @param dy Normalized vertical mouse delta
     * @param focal_distance Current focal distance (controls pan speed)
     */
    static auto compute_pan(
        const std::array<float, 3>& current_position,
        const std::array<float, 4>& current_orientation,
        float dx, float dy,
        float focal_distance) -> PanResult;

    /**
     * @brief Compute zoom along view direction (perspective) or height change (orthographic).
     * @param current_position Current camera world position
     * @param current_orientation Current camera orientation quaternion
     * @param focal_distance Current focal distance
     * @param delta Zoom amount (positive = zoom in)
     * @param is_orthographic Whether to use orthographic zoom mode
     * @param ortho_height Current orthographic viewport height
     */
    static auto compute_zoom(
        const std::array<float, 3>& current_position,
        const std::array<float, 4>& current_orientation,
        float focal_distance,
        float delta,
        bool is_orthographic,
        float ortho_height) -> ZoomResult;
};

} // namespace OpenGeoLab::Render
