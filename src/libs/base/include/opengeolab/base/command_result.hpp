/**
 * @file command_result.hpp
 * @brief Provides the structured result type returned by command-oriented interfaces.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace OpenGeoLab::Base {

/**
 * @brief Describes the outcome of a command execution.
 */
struct CommandResult {
    bool ok{true};
    std::string summary;
    nlohmann::json result;
};

} // namespace OpenGeoLab::Base
