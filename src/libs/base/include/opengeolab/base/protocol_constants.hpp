/**
 * @file protocol_constants.hpp
 * @brief Defines protocol-level constants shared by all command dispatchers.
 */

#pragma once

#include <string_view>

namespace OpenGeoLab::Base {

/// @brief Current protocol version for command response envelopes.
inline constexpr std::string_view kProtocolVersion = "1.0";

} // namespace OpenGeoLab::Base
