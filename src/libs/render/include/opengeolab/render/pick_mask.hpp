/**
 * @file pick_mask.hpp
 * @brief Compatibility header — PickMask now lives in core
 */

#pragma once

#include <opengeolab/core/pick_mask.hpp>

namespace OpenGeoLab::Render {

using OpenGeoLab::Core::PickMask;
using OpenGeoLab::Core::PickMode;

} // namespace OpenGeoLab::Render
