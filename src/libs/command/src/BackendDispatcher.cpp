#include <opengeolab/command/BackendDispatcher.hpp>
#include <opengeolab/command/JsonProtocol.hpp>
#include <opengeolab/geometry/GeometryService.hpp>
#include <opengeolab/interaction/InteractionRecorder.hpp>
#include <opengeolab/render/RenderService.hpp>
#include <opengeolab/selection/SelectionService.hpp>

#include <fmt/format.h>

#include <stdexcept>

namespace OpenGeoLab::Command
{

namespace
{

[[nodiscard]] nlohmann::json makeCapabilities()
{
    return {
        {"protocolVersion", PROCESS_PROTOCOL_VERSION},
        {"entrypoints",
         {{"qml", true}, {"embeddedPython", true}, {"pyWrapperModule", true}, {"externalLlm", true}}},
        {"render",
         {{"snapshot", true}, {"viewportDescription", true}, {"cameraStateRecording", true}}},
        {"selection",
         {{"pickDescription", true}, {"boxDescription", true}, {"headlessQuery", true}}},
        {"interaction",
         {{"recordOperation", true}, {"exportPython", true}, {"replayPlan", true}}},
        {"plugins",
         {{"pythonDiscovery", true}, {"pyside6UiMetadata", true}, {"launchUiAction", true}}},
        {"supportedActions", BackendDispatcher::supportedActions()}
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
           {"action",
            "system.ping|geometry.box.describe|render.viewport.describe|render.snapshot.capture|"
            "selection.pick.describe|selection.box.describe|interaction.record.operation|"
            "interaction.export.python|interaction.replay.describe"},
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

[[nodiscard]] ResponseEnvelope handleViewportDescription(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Viewport state normalized for replay.";
    response.result = OpenGeoLab::Render::RenderService::describeViewport(request.payload);
    return response;
}

[[nodiscard]] ResponseEnvelope handleSnapshot(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Placeholder render snapshot generated.";
    response.result = OpenGeoLab::Render::RenderService::captureSnapshot(request.payload);
    return response;
}

[[nodiscard]] ResponseEnvelope handleSelectionPick(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Selection pick query normalized.";
    response.result = OpenGeoLab::Selection::SelectionService::describePick(request.payload);
    return response;
}

[[nodiscard]] ResponseEnvelope handleSelectionBox(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Selection box query normalized.";
    response.result = OpenGeoLab::Selection::SelectionService::describeBoxSelection(request.payload);
    return response;
}

[[nodiscard]] ResponseEnvelope handleRecordOperation(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Interaction operations normalized for recording.";
    response.result = OpenGeoLab::Interaction::InteractionRecorder::recordOperation(request.payload);
    return response;
}

[[nodiscard]] ResponseEnvelope handleExportPython(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Replay-oriented Python script generated.";
    response.result = OpenGeoLab::Interaction::InteractionRecorder::exportPythonScript(request.payload);
    return response;
}

[[nodiscard]] ResponseEnvelope handleReplayPlan(const RequestEnvelope& request)
{
    auto response = makeBaseResponse(request);
    response.ok = true;
    response.summary = "Replay plan generated.";
    response.result = OpenGeoLab::Interaction::InteractionRecorder::describeReplayPlan(request.payload);
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

    if (request.action == "render.viewport.describe") {
        return handleViewportDescription(request);
    }

    if (request.action == "render.snapshot.capture") {
        return handleSnapshot(request);
    }

    if (request.action == "selection.pick.describe") {
        return handleSelectionPick(request);
    }

    if (request.action == "selection.box.describe") {
        return handleSelectionBox(request);
    }

    if (request.action == "interaction.record.operation") {
        return handleRecordOperation(request);
    }

    if (request.action == "interaction.export.python") {
        return handleExportPython(request);
    }

    if (request.action == "interaction.replay.describe") {
        return handleReplayPlan(request);
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
        "render.viewport.describe",
        "render.snapshot.capture",
        "selection.pick.describe",
        "selection.box.describe",
        "interaction.record.operation",
        "interaction.export.python",
        "interaction.replay.describe"
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
