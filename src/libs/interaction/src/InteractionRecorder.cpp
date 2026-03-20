#include <opengeolab/interaction/InteractionRecorder.hpp>

#include <opengeolab/render/RenderService.hpp>
#include <opengeolab/selection/SelectionService.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Interaction
{

namespace
{

const nlohmann::json& ensureObject(
    const nlohmann::json& payload,
    std::string_view context
)
{
    if (!payload.is_object()) {
        throw std::invalid_argument(std::string(context) + " must be a JSON object");
    }

    return payload;
}

[[nodiscard]] std::vector<nlohmann::json> collectOperations(const nlohmann::json& payload)
{
    const auto& object_payload = ensureObject(payload, "interaction payload");

    if (object_payload.contains("operations")) {
        const auto& operations = object_payload.at("operations");
        if (!operations.is_array()) {
            throw std::invalid_argument("operations must be a JSON array");
        }

        std::vector<nlohmann::json> collected;
        collected.reserve(operations.size());
        for (const auto& operation : operations) {
            ensureObject(operation, "interaction operation");
            collected.push_back(operation);
        }
        return collected;
    }

    if (object_payload.contains("operation")) {
        const auto& operation = object_payload.at("operation");
        ensureObject(operation, "interaction operation");
        return {operation};
    }

    return {object_payload};
}

[[nodiscard]] std::string replayCategory(std::string_view kind)
{
    if (kind.starts_with("camera.")) {
        return "view-state";
    }

    if (kind.starts_with("selection.")) {
        return "selection-query";
    }

    return "semantic-command";
}

[[nodiscard]] std::string replayReason(std::string_view kind)
{
    if (kind.starts_with("camera.")) {
        return "Replay the explicit camera state instead of device-relative orbit input.";
    }

    if (kind.starts_with("selection.")) {
        return "Replay semantic selection with the recorded view state and filters.";
    }

    return "Replay a semantic command boundary rather than a UI gesture.";
}

[[nodiscard]] nlohmann::json normalizeOperation(const nlohmann::json& operation)
{
    const auto& object_payload = ensureObject(operation, "interaction operation");
    const std::string kind = object_payload.value("kind", "command.execute");

    nlohmann::json semantic_payload = nlohmann::json::object();
    if (kind == "camera.orbit" || kind == "camera.restore") {
        semantic_payload = OpenGeoLab::Render::RenderService::describeViewport(
            object_payload.value("view", nlohmann::json::object())
        );
    }
    else if (kind == "selection.box") {
        semantic_payload = OpenGeoLab::Selection::SelectionService::describeBoxSelection(
            object_payload.value("selection", object_payload)
        );
    }
    else if (kind == "selection.pick") {
        semantic_payload = OpenGeoLab::Selection::SelectionService::describePick(
            object_payload.value("selection", object_payload)
        );
    }
    else {
        semantic_payload = object_payload.value("payload", nlohmann::json::object());
    }

    return {
        {"operationId", object_payload.value("operationId", kind)},
        {"kind", kind},
        {"uiSource", object_payload.value("uiSource", "qml")},
        {"semanticPayload", semantic_payload},
        {"result", object_payload.value("result", nlohmann::json::object())},
        {"replayBoundary",
         {{"category", replayCategory(kind)},
          {"recordRawInput", false},
          {"headlessReady", true},
          {"reason", replayReason(kind)}}}
    };
}

[[nodiscard]] std::string escapePythonString(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size() + 4);
    escaped.push_back('\'');
    for (const char character : value) {
        if (character == '\\' || character == '\'') {
            escaped.push_back('\\');
        }
        escaped.push_back(character);
    }
    escaped.push_back('\'');
    return escaped;
}

[[nodiscard]] std::string pythonStringList(const std::vector<std::string>& values)
{
    std::string rendered {"["};
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            rendered += ", ";
        }
        rendered += escapePythonString(values[index]);
    }
    rendered += "]";
    return rendered;
}

[[nodiscard]] std::string joinLines(const std::vector<std::string>& lines)
{
    std::string result;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        result += lines[index];
        if (index + 1 < lines.size()) {
            result += '\n';
        }
    }
    return result;
}

[[nodiscard]] std::string buildReplayAction(std::string_view kind)
{
    if (kind == "camera.orbit" || kind == "camera.restore") {
        return "restore_viewport";
    }

    if (kind == "selection.box") {
        return "box_select";
    }

    if (kind == "selection.pick") {
        return "pick_select";
    }

    return "run_command";
}

}  // namespace

nlohmann::json InteractionRecorder::recordOperation(const nlohmann::json& payload)
{
    const auto operations = collectOperations(payload);

    auto recorded_operations = nlohmann::json::array();
    for (const auto& operation : operations) {
        recorded_operations.push_back(normalizeOperation(operation));
    }

    return {
        {"operationCount", recorded_operations.size()},
        {"headlessReady", true},
        {"operations", recorded_operations}
    };
}

nlohmann::json InteractionRecorder::exportPythonScript(const nlohmann::json& payload)
{
    const auto operations = collectOperations(payload);

    std::vector<std::string> lines {
        "from __future__ import annotations",
        "",
        "",
        "def replay(api) -> None:"
    };

    if (operations.empty()) {
        lines.emplace_back("    pass");
    }

    for (const auto& operation : operations) {
        const auto normalized = normalizeOperation(operation);
        const std::string kind = normalized.at("kind").get<std::string>();

        if (kind == "camera.orbit" || kind == "camera.restore") {
            const auto& view = normalized.at("semanticPayload");
            const auto& camera = view.at("camera");
            const auto& target = camera.at("target");

            lines.push_back(
                fmt::format(
                    "    api.restore_viewport(viewport_id={}, camera_model={}, target=({}, {}, {}), "
                    "distance={}, azimuth_deg={}, elevation_deg={}, roll_deg={})",
                    escapePythonString(view.at("viewportId").get<std::string>()),
                    escapePythonString(view.at("cameraModel").get<std::string>()),
                    target.at("x").get<double>(),
                    target.at("y").get<double>(),
                    target.at("z").get<double>(),
                    camera.at("distance").get<double>(),
                    camera.at("azimuthDeg").get<double>(),
                    camera.at("elevationDeg").get<double>(),
                    camera.at("rollDeg").get<double>()
                )
            );
            continue;
        }

        if (kind == "selection.box") {
            const auto& selection = normalized.at("semanticPayload");
            const auto& rectangle = selection.at("rectangle");
            const auto entity_kinds
                = selection.at("filters").at("entityKinds").get<std::vector<std::string>>();

            lines.push_back(
                fmt::format(
                    "    api.box_select(viewport_id={}, rectangle=({}, {}, {}, {}), "
                    "entity_kinds={}, replace={})",
                    escapePythonString(selection.at("viewport").at("viewportId").get<std::string>()),
                    rectangle.at("left").get<int>(),
                    rectangle.at("top").get<int>(),
                    rectangle.at("right").get<int>(),
                    rectangle.at("bottom").get<int>(),
                    pythonStringList(entity_kinds),
                    selection.at("selectionIntent").at("operation").get<std::string>() == "replace"
                        ? "True"
                        : "False"
                )
            );
            continue;
        }

        if (kind == "selection.pick") {
            const auto& selection = normalized.at("semanticPayload");
            const auto entity_kinds
                = selection.at("filters").at("entityKinds").get<std::vector<std::string>>();

            lines.push_back(
                fmt::format(
                    "    api.pick_select(viewport_id={}, screen_position=({}, {}), entity_kinds={})",
                    escapePythonString(selection.at("viewport").at("viewportId").get<std::string>()),
                    selection.at("screenPosition").at("x").get<int>(),
                    selection.at("screenPosition").at("y").get<int>(),
                    pythonStringList(entity_kinds)
                )
            );
            continue;
        }

        lines.push_back(
            fmt::format(
                "    api.run_command(name={})",
                escapePythonString(operation.value("command", kind))
            )
        );
    }

    return {
        {"language", "python"},
        {"entrypoint", "replay(api)"},
        {"operationCount", operations.size()},
        {"headlessReady", true},
        {"script", joinLines(lines)}
    };
}

nlohmann::json InteractionRecorder::describeReplayPlan(const nlohmann::json& payload)
{
    const auto operations = collectOperations(payload);

    auto steps = nlohmann::json::array();
    for (std::size_t index = 0; index < operations.size(); ++index) {
        const auto normalized = normalizeOperation(operations[index]);
        const std::string kind = normalized.at("kind").get<std::string>();

        steps.push_back({
            {"stepIndex", index},
            {"kind", kind},
            {"replayAction", buildReplayAction(kind)},
            {"headlessReady", normalized.at("replayBoundary").at("headlessReady")},
            {"semanticPayload", normalized.at("semanticPayload")},
            {"result", normalized.at("result")}
        });
    }

    return {
        {"stepCount", steps.size()},
        {"headlessReady", true},
        {"stableBoundaryAdvice",
         "Replay explicit view state and semantic commands instead of raw UI events."},
        {"steps", steps}
    };
}

}  // namespace OpenGeoLab::Interaction
