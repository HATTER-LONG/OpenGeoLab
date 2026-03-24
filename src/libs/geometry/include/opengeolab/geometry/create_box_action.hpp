/**
 * @file create_box_action.hpp
 * @brief Action that creates a box with simulated progress reporting.
 */
#pragma once

#include <opengeolab/geometry/box_data.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <functional>
#include <string_view>

namespace OpenGeoLab::Geometry {

/**
 * @brief Progress callback signature for geometry actions.
 * @param progress Normalized progress [0, 1].
 * @param message Human-readable status message.
 */
using ProgressCallback = std::function<void(double, std::string_view)>;

/**
 * @brief Create a box with simulated vertex generation.
 *
 * Simulates a long-running operation by sleeping between steps.
 * Reports progress via callback and sends notifications via the
 * global NotificationRegistry sink.
 *
 * @param center Box center coordinates [x, y, z].
 * @param dimensions Box dimensions [w, h, d].
 * @param vertex_count Number of vertices to simulate generating.
 * @param progress_callback Optional callback for progress reporting.
 * @return The generated BoxData.
 */
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT BoxData
createBox(std::array<double, 3> center,
          std::array<double, 3> dimensions,
          int vertex_count,
          const ProgressCallback& progress_callback = {});

} // namespace OpenGeoLab::Geometry
