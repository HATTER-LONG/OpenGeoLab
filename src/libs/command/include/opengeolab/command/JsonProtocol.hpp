/**
 * @file JsonProtocol.hpp
 * @brief Defines the lightweight JSON process envelope shared by QML, Python, and C++.
 */

#pragma once

#include <opengeolab/command/CommandExport.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace OpenGeoLab::Command
{

inline constexpr std::string_view PROCESS_PROTOCOL_VERSION {"1.0"};

/**
 * @brief Represents a normalized backend process request.
 */
struct RequestEnvelope
{
    std::string protocolVersion {std::string(PROCESS_PROTOCOL_VERSION)};
    std::string requestId;
    std::string source {"unknown"};
    std::string action;
    nlohmann::json payload = nlohmann::json::object();
    nlohmann::json context = nlohmann::json::object();
};

/**
 * @brief Represents a normalized backend process response.
 */
struct ResponseEnvelope
{
    std::string protocolVersion {std::string(PROCESS_PROTOCOL_VERSION)};
    std::string requestId;
    bool ok {false};
    std::string action;
    std::string summary;
    nlohmann::json result = nlohmann::json::object();
    nlohmann::json diagnostics = nlohmann::json::object();
    nlohmann::json errors = nlohmann::json::array();
};

/**
 * @brief Parses an incoming JSON request string into a strongly typed envelope.
 * @param request_json Raw JSON request string.
 * @return Parsed request envelope.
 */
[[nodiscard]] OPENGEOLAB_COMMAND_EXPORT RequestEnvelope parseRequest(std::string_view request_json);

/**
 * @brief Converts a response envelope into JSON.
 * @param response Response envelope to convert.
 * @return JSON representation of the response envelope.
 */
[[nodiscard]] OPENGEOLAB_COMMAND_EXPORT nlohmann::json toJson(const ResponseEnvelope& response);

/**
 * @brief Serializes a response envelope into an indented JSON string.
 * @param response Response envelope to serialize.
 * @return Indented JSON string for transport to Python or QML.
 */
[[nodiscard]] OPENGEOLAB_COMMAND_EXPORT std::string
serializeResponse(const ResponseEnvelope& response);

}  // namespace OpenGeoLab::Command
