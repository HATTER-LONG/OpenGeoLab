/**
 * @file pick_action.hpp
 * @brief PickAction — intent of a user pick gesture
 */

#pragma once

#include <cstdint>

namespace OpenGeoLab::Core {

/// Describes whether a pick gesture adds or removes from selection.
enum class PickAction : uint8_t {
    Add,    ///< Left-click: add entity to selection
    Remove, ///< Right-click: remove entity from selection
};

} // namespace OpenGeoLab::Core
