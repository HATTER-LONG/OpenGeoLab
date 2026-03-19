/**
 * @file BackendDispatcher.hpp
 * @brief Routes JSON process requests to lightweight backend services.
 */

#pragma once

#include <opengeolab/command/CommandExport.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Command
{

/**
 * @brief Provides the unified JSON process entry point for Python and UI clients.
 */
class OPENGEOLAB_COMMAND_EXPORT BackendDispatcher
{
public:
    /**
     * @brief Processes a JSON request and returns a JSON response.
     * @param request_json Raw JSON request string.
     * @return JSON response string.
     */
    [[nodiscard]] static std::string process(std::string_view request_json);

    /**
     * @brief Lists the actions currently supported by the backend skeleton.
     * @return Supported action names.
     */
    [[nodiscard]] static std::vector<std::string> supportedActions();
};

}  // namespace OpenGeoLab::Command
