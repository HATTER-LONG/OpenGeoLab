/// @file geometry_module.hpp
/// @brief JSON dispatcher for the geometry module.
#pragma once

#include <opengeolab/geometry/geometry_export.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace OpenGeoLab::Geometry {

/// @brief Progress callback for geometry module operations.
using ModuleProgressCallback = std::function<void(double, std::string_view)>;

/// @brief Process a JSON request for the geometry module.
///
/// Routes actions to the appropriate geometry operations.
/// Currently supports: "create_box".
///
/// @param request_json Full JSON request envelope.
/// @param progress_callback Optional progress reporting callback.
/// @return JSON response string.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT std::string
processGeometry(std::string_view request_json, ModuleProgressCallback progress_callback = nullptr);

} // namespace OpenGeoLab::Geometry
