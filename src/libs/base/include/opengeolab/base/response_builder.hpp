/**
 * @file response_builder.hpp
 * @brief Provides reusable JSON response envelope builders for command dispatchers.
 */

#pragma once

#include <opengeolab/base/base_export.hpp>

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <string_view>

namespace OpenGeoLab::Base {

/// @brief Builds a standard JSON response envelope with module field.
[[nodiscard]] OPENGEOLAB_BASE_EXPORT auto makeResponse(const nlohmann::json& request_id,
                                                       const nlohmann::json& module,
                                                       const nlohmann::json& action,
                                                       bool ok,
                                                       std::string_view summary,
                                                       const nlohmann::json& result,
                                                       const nlohmann::json& errors) -> std::string;

/// @brief Builds a legacy JSON response envelope without module field.
[[nodiscard]] OPENGEOLAB_BASE_EXPORT auto makeResponse(const nlohmann::json& request_id,
                                                       const nlohmann::json& action,
                                                       bool ok,
                                                       std::string_view summary,
                                                       const nlohmann::json& result,
                                                       const nlohmann::json& errors) -> std::string;

/// @brief Builds a module-level error response using structured error items.
[[nodiscard]] OPENGEOLAB_BASE_EXPORT auto makeErrorResponse(const nlohmann::json& request_id,
                                                            const nlohmann::json& module,
                                                            const nlohmann::json& action,
                                                            std::string_view message)
    -> std::string;

/// @brief Builds a legacy error response using plain string error array.
[[nodiscard]] OPENGEOLAB_BASE_EXPORT auto makeErrorResponse(const nlohmann::json& request_id,
                                                            const nlohmann::json& action,
                                                            std::string_view message)
    -> std::string;

/// @brief Builds a single structured error item.
[[nodiscard]] OPENGEOLAB_BASE_EXPORT auto makeErrorItem(std::string_view message) -> nlohmann::json;

} // namespace OpenGeoLab::Base
