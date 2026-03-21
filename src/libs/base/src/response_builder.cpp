#include <opengeolab/base/response_builder.hpp>

#include <opengeolab/base/protocol_constants.hpp>

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Base {

auto makeResponse(const nlohmann::json& request_id,
                  const nlohmann::json& module,
                  const nlohmann::json& action,
                  bool ok,
                  std::string_view summary,
                  const nlohmann::json& result,
                  const nlohmann::json& errors) -> std::string {
    return nlohmann::json{{"protocolVersion", kProtocolVersion},
                          {"requestId", request_id},
                          {"ok", ok},
                          {"module", module},
                          {"action", action},
                          {"summary", summary},
                          {"result", result},
                          {"errors", errors}}
        .dump();
}

auto makeResponse(const nlohmann::json& request_id,
                  const nlohmann::json& action,
                  bool ok,
                  std::string_view summary,
                  const nlohmann::json& result,
                  const nlohmann::json& errors) -> std::string {
    return nlohmann::json{{"protocolVersion", kProtocolVersion},
                          {"requestId", request_id},
                          {"ok", ok},
                          {"action", action},
                          {"summary", summary},
                          {"result", result},
                          {"errors", errors}}
        .dump();
}

auto makeErrorResponse(const nlohmann::json& request_id,
                       const nlohmann::json& module,
                       const nlohmann::json& action,
                       std::string_view message) -> std::string {
    return makeResponse(request_id, module, action, false, message, nlohmann::json::object(),
                        nlohmann::json::array({makeErrorItem(message)}));
}

auto makeErrorResponse(const nlohmann::json& request_id,
                       const nlohmann::json& action,
                       std::string_view message) -> std::string {
    return makeResponse(request_id, action, false, message, nlohmann::json::object(),
                        nlohmann::json::array({message}));
}

auto makeErrorItem(std::string_view message) -> nlohmann::json {
    return nlohmann::json{{"message", message}};
}

} // namespace OpenGeoLab::Base
