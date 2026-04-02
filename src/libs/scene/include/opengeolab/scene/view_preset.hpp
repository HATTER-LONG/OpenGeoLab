/**
 * @file view_preset.hpp
 * @brief Standard camera view presets
 */

#pragma once

#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

/// @brief 7 standard camera view presets.
enum class ViewPreset { Front, Back, Top, Bottom, Left, Right, Isometric };

} // namespace OpenGeoLab::Scene
