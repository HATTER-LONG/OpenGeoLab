#include <ogl/command/RequestProtocol.hpp>

#include <stdexcept>
#include <string>

namespace OGL::Command {

auto normalizeRequestParamObject(const nlohmann::json& requestJson) -> nlohmann::json {
    if(!requestJson.is_object()) {
        throw std::invalid_argument("Request payload must be a JSON object.");
    }

    nlohmann::json normalizedRequest = requestJson;
    if(!normalizedRequest.contains("param") || normalizedRequest.at("param").is_null()) {
        normalizedRequest["param"] = nlohmann::json::object();
    }

    if(!normalizedRequest.at("param").is_object()) {
        throw std::invalid_argument("Request param must be a JSON object.");
    }

    return normalizedRequest;
}

auto parseCommandRequest(const nlohmann::json& requestJson) -> CommandRequest {
    const auto normalizedRequest = normalizeRequestParamObject(requestJson);
    const std::string module = normalizedRequest.value("module", std::string{});
    if(module.empty()) {
        throw std::invalid_argument("Request module cannot be empty.");
    }

    const std::string action = normalizedRequest.value("action", std::string{});
    if(action.empty()) {
        throw std::invalid_argument("Request action cannot be empty.");
    }

    return {.module = module, .action = action, .param = normalizedRequest.at("param")};
}

} // namespace OGL::Command
