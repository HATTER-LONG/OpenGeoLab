#include <opengeolab/command/BackendDispatcher.hpp>
#include <opengeolab/command/JsonProtocol.hpp>
#include <opengeolab/geometry/GeometryService.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <stdexcept>

namespace OpenGeoLab::Command
{

namespace
{

constexpr std::string_view PLACEHOLDER_PNG_BASE64 {
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+c1ioAAAAASUVORK5CYII="
};

[[nodiscard]] nlohmann::json makeCapabilities()
{
    return {
        {"protocolVersion", PROCESS_PROTOCOL_VERSION},
        {"entrypoints",
         {{"qml", true}, {"embeddedPython", true}, {"pyWrapperModule", true}, {"externalLlm", true}}},
        {"render",
         {{"snapshot", true},
          {"selectionIntrospection", true},
          {"implementation", "placeholder-freecad-inspired-contract"}}},
        {"plugins",
         {{"pythonDiscovery", true}, {"pyside6UiMetadata", true}, {"launchUiAction", true}}},
        {"supportedActions", BackendDispatcher::supportedActions()}
    };
}

[[nodiscard]] nlohmann::json makePlaceholderSnapshot()
{
    return {
        {"mimeType", "image/png"},
        {"encoding", "base64"},
        {"width", 1},
        {"height", 1},
        {"data", PLACEHOLDER_PNG_BASE64},
        {"summary", "Placeholder viewport snapshot used to validate the protocol end-to-end."}
    };
}

[[nodiscard]] ResponseEnvelope makeBaseResponse(const RequestEnvelope& request)
{
    ResponseEnvelope response;
    response.requestId = request.requestId;
    response.action = request.action;
    response.diagnostics = {
        {"source", request.source},
        {"backend", "OpenGeoLab skeleton"},
        {"capabilities", makeCapabilities()}
    };
    return response;
}

[[nodiscard]] ResponseEnvelope handlePing(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "OpenGeoLab backend is reachable.";
    response.result = {
        {"message", "Skeleton bridge is alive."},
        {"capabilities", makeCapabilities()}
    };
    return response;
}

[[nodiscard]] ResponseEnvelope handleDescribeProtocol(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Protocol schema returned.";
    response.result = {
        {"request",
         {{"protocolVersion", PROCESS_PROTOCOL_VERSION},
          {"requestId", "string"},
          {"source", "qml-shell|python-plugin|llm"},
          {"action", "system.ping|geometry.box.describe|render.snapshot.capture|selection.pick.describe"},
          {"payload", "json object"},
          {"context", "json object"}}},
        {"response",
         {{"protocolVersion", PROCESS_PROTOCOL_VERSION},
          {"requestId", "string"},
          {"ok", "bool"},
          {"action", "string"},
          {"summary", "string"},
          {"result", "json object"},
          {"diagnostics", "json object"},
          {"errors", "json array"}}}
    };
    return response;
}

[[nodiscard]] ResponseEnvelope handleGeometryBox(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Geometry box description computed.";
    response.result = OpenGeoLab::Geometry::GeometryService::describeBox(request.payload);
    return response;
}

[[nodiscard]] ResponseEnvelope handleSnapshot(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Placeholder render snapshot generated.";
    response.result = {
        {"snapshot", makePlaceholderSnapshot()},
        {"viewport",
         {{"renderer", "placeholder"},
          {"cameraModel", "orbit"},
          {"selectionMode", "single-pick"},
          {"futureHooks", {"syncScene", "boxSelect", "gpuSnapshot"}}}}
    };
    return response;
}

[[nodiscard]] ResponseEnvelope handleSelectionPick(const RequestEnvelope& request)
{
    const int screen_x = request.payload.value("screenX", 0);
    const int screen_y = request.payload.value("screenY", 0);

    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Placeholder selection payload returned.";
    response.result = {
        {"screenPosition", {{"x", screen_x}, {"y", screen_y}}},
        {"worldPosition", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
        {"hit", {{"entityId", "box://demo/0"}, {"subElement", "Face1"}, {"confidence", 0.0}}},
        {"selectionIntent",
         {{"style", "single-pick"},
          {"inspiredBy", "FreeCAD unified selection root + application-level selection state"}}}
    };
    return response;
}

[[nodiscard]] ResponseEnvelope makeUnsupportedActionResponse(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = false;
    response.summary = fmt::format("Unsupported action '{}'.", request.action);
    response.errors = nlohmann::json::array(
        {nlohmann::json {{"message", response.summary}, {"supportedActions", BackendDispatcher::supportedActions()}}}
    );
    return response;
}

[[nodiscard]] ResponseEnvelope dispatch(const RequestEnvelope& request)
{
    if (request.action == "system.ping") {
        return handlePing(request);
    }

    if (request.action == "protocol.describe") {
        return handleDescribeProtocol(request);
    }

    if (request.action == "geometry.box.describe") {
        return handleGeometryBox(request);
    }

    if (request.action == "render.snapshot.capture") {
        return handleSnapshot(request);
    }

    if (request.action == "selection.pick.describe") {
        return handleSelectionPick(request);
    }

    return makeUnsupportedActionResponse(request);
}

}  // namespace

RequestEnvelope parseRequest(std::string_view request_json)
{
    const auto request_value = nlohmann::json::parse(request_json.begin(), request_json.end());
    if (!request_value.is_object()) {
        throw std::invalid_argument("Request must be a JSON object");
    }

    RequestEnvelope request;
    request.protocolVersion
        = request_value.value("protocolVersion", std::string(PROCESS_PROTOCOL_VERSION));
    request.requestId = request_value.value("requestId", "generated-request");
    request.source = request_value.value("source", "unknown");
    request.action = request_value.value("action", "");
    request.payload = request_value.value("payload", nlohmann::json::object());
    request.context = request_value.value("context", nlohmann::json::object());

    if (request.action.empty()) {
        throw std::invalid_argument("Request action must not be empty");
    }

    return request;
}

nlohmann::json toJson(const ResponseEnvelope& response)
{
    return {
        {"protocolVersion", response.protocolVersion},
        {"requestId", response.requestId},
        {"ok", response.ok},
        {"action", response.action},
        {"summary", response.summary},
        {"result", response.result},
        {"diagnostics", response.diagnostics},
        {"errors", response.errors}
    };
}

std::string serializeResponse(const ResponseEnvelope& response)
{
    return toJson(response).dump(2);
}

std::vector<std::string> BackendDispatcher::supportedActions()
{
    return {
        "system.ping",
        "protocol.describe",
        "geometry.box.describe",
        "render.snapshot.capture",
        "selection.pick.describe"
    };
}

std::string BackendDispatcher::process(std::string_view request_json)
{
    try {
        const auto request = parseRequest(request_json);
        return serializeResponse(dispatch(request));
    }
    catch (const std::invalid_argument& error) {
        ResponseEnvelope response;
        response.ok = false;
        response.summary = "Request validation failed.";
        response.errors
            = nlohmann::json::array({nlohmann::json {{"message", error.what()}}});
        return serializeResponse(response);
    }
    catch (const nlohmann::json::exception& error) {
        ResponseEnvelope response;
        response.ok = false;
        response.summary = "JSON parsing failed.";
        response.errors
            = nlohmann::json::array({nlohmann::json {{"message", error.what()}}});
        return serializeResponse(response);
    }
}

}  // namespace OpenGeoLab::Command
