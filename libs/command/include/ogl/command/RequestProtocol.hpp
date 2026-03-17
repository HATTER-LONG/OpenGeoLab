/**
 * @file RequestProtocol.hpp
 * @brief Shared helpers for validating and normalizing command request envelopes.
 */

#pragma once

#include <ogl/command/CommandService.hpp>
#include <ogl/command/export.hpp>

#include <nlohmann/json.hpp>

namespace OGL::Command {

/**
 * @brief Normalize the public request envelope to always contain an object-valued `param`.
 * @param requestJson Raw request JSON from UI, Python, or tests.
 * @return Normalized JSON object preserving extra top-level keys.
 * @throws std::invalid_argument When the request root is not an object or `param` is not an object.
 */
OGL_COMMAND_EXPORT auto normalizeRequestParamObject(const nlohmann::json& requestJson)
    -> nlohmann::json;

/**
 * @brief Parse a raw request envelope into the canonical command request shape.
 * @param requestJson Raw request JSON from UI, Python, or tests.
 * @return Parsed command request with normalized object-valued `param`.
 * @throws std::invalid_argument When the request envelope is malformed.
 */
OGL_COMMAND_EXPORT auto parseCommandRequest(const nlohmann::json& requestJson) -> CommandRequest;

} // namespace OGL::Command
