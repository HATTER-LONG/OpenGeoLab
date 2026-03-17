#include "OpenGeoLabFeedbackCoordinator.hpp"

#include <nlohmann/json.hpp>

namespace OGL::App {

namespace {

auto normalizedOperationScope(const QString& scope) -> QString {
    const QString trimmedScope = scope.trimmed();
    return trimmedScope.isEmpty() ? QStringLiteral("app") : trimmedScope;
}

auto normalizedOperationMessage(const QString& message) -> QString {
    const QString trimmedMessage = message.trimmed();
    return trimmedMessage.isEmpty() ? QStringLiteral("Operation updated.") : trimmedMessage;
}

auto secondaryOperationDetail(const QString& detail, const QString& primaryMessage) -> QString {
    const QString trimmedDetail = detail.trimmed();
    if(trimmedDetail.isEmpty() || trimmedDetail == primaryMessage.trimmed()) {
        return QString{};
    }
    return trimmedDetail;
}

auto requestScope(const QString& module, const QString& action) -> QString {
    if(module.trimmed().isEmpty()) {
        return normalizedOperationScope(action);
    }
    if(action.trimmed().isEmpty()) {
        return normalizedOperationScope(module);
    }
    return normalizedOperationScope(
        QStringLiteral("%1.%2").arg(module.trimmed(), action.trimmed()));
}

auto requestMessage(const RequestStartedEvent& event) -> QString {
    return QStringLiteral("Running %1...").arg(requestScope(event.module, event.action));
}

void copyIfPresent(const nlohmann::json& source,
                   nlohmann::json& target,
                   const char* sourceKey,
                   const char* targetKey = nullptr) {
    if(source.contains(sourceKey)) {
        target[targetKey == nullptr ? sourceKey : targetKey] = source.at(sourceKey);
    }
}

struct JsonFieldSpec {
    const char* sourceKey;
    const char* targetKey{nullptr};
};

auto buildObjectSubset(const nlohmann::json& source, std::initializer_list<JsonFieldSpec> fields)
    -> nlohmann::json {
    nlohmann::json subset = nlohmann::json::object();
    if(!source.is_object()) {
        return subset;
    }

    for(const auto& field : fields) {
        copyIfPresent(source, subset, field.sourceKey, field.targetKey);
    }
    return subset;
}

void copyNestedFieldsFlat(const nlohmann::json& payload,
                          nlohmann::json& target,
                          const char* payloadKey,
                          std::initializer_list<JsonFieldSpec> fields) {
    const auto subset =
        buildObjectSubset(payload.value(payloadKey, nlohmann::json::object()), fields);
    for(const auto& [key, value] : subset.items()) {
        target[key] = value;
    }
}

void copyNestedObjectSubset(const nlohmann::json& payload,
                            nlohmann::json& target,
                            const char* payloadKey,
                            std::initializer_list<JsonFieldSpec> fields) {
    auto subset = buildObjectSubset(payload.value(payloadKey, nlohmann::json::object()), fields);
    if(!subset.empty()) {
        target[payloadKey] = std::move(subset);
    }
}

auto buildUiPayloadPreview(const nlohmann::json& payload) -> nlohmann::json {
    nlohmann::json preview = nlohmann::json::object();
    copyIfPresent(payload, preview, "summary");
    copyIfPresent(payload, preview, "shapeType");
    copyIfPresent(payload, preview, "recordedCommandCount");
    copyNestedFieldsFlat(payload, preview, "model", {{"modelName"}, {"bodyCount"}});
    copyNestedFieldsFlat(payload, preview, "geometryModel", {{"modelName"}, {"bodyCount"}});
    copyNestedFieldsFlat(payload, preview, "sceneGraph", {{"sceneId"}, {"nodeCount"}});
    copyNestedFieldsFlat(payload, preview, "renderFrame", {{"frameId"}, {"drawItemCount"}});
    copyNestedFieldsFlat(payload, preview, "selectionResult", {{"mode"}, {"hitCount"}});
    copyNestedFieldsFlat(payload, preview, "replayReport", {{"replayedCount"}, {"successCount"}});
    return preview;
}

auto buildUiResponsePreview(const nlohmann::json& response) -> nlohmann::json {
    const auto payload = response.value("payload", nlohmann::json::object());
    return {{"success", response.value("success", false)},
            {"module", response.value("module", std::string{})},
            {"action", response.value("action", std::string{})},
            {"message", response.value("message", std::string{})},
            {"payload", buildUiPayloadPreview(payload)}};
}

} // namespace

auto OpenGeoLabFeedbackCoordinator::buildPublicResponse(const nlohmann::json& response)
    -> nlohmann::json {
    const auto success = response.value("success", false);
    const auto module = response.value("module", std::string{});
    const auto action = response.value("action", std::string{});
    const auto payload = response.value("payload", nlohmann::json::object());

    nlohmann::json publicPayload = nlohmann::json::object();
    copyIfPresent(payload, publicPayload, "shapeType");
    copyIfPresent(payload, publicPayload, "recordedCommandCount");
    copyNestedObjectSubset(payload, publicPayload, "model",
                           {{"modelName"}, {"bodyCount"}, {"source"}});
    copyNestedObjectSubset(payload, publicPayload, "geometryModel",
                           {{"modelName"}, {"bodyCount"}, {"source"}});
    copyNestedObjectSubset(payload, publicPayload, "sceneGraph", {{"sceneId"}, {"nodeCount"}});
    copyNestedObjectSubset(payload, publicPayload, "renderFrame", {{"frameId"}, {"drawItemCount"}});
    copyNestedObjectSubset(payload, publicPayload, "selectionResult", {{"mode"}, {"hitCount"}});
    copyNestedObjectSubset(payload, publicPayload, "replayReport",
                           {{"replayedCount"}, {"successCount"}});

    const std::string publicSummary =
        !module.empty() && !action.empty()
            ? (module + " " + action +
               (success ? " completed successfully." : " failed to complete."))
            : response.value("message", std::string{"Request completed."});

    return {
        {"success", success}, {"summary", publicSummary}, {"payload", std::move(publicPayload)}};
}

auto OpenGeoLabFeedbackCoordinator::onRequestStarted(const RequestStartedEvent& event) const
    -> ControllerStateDelta {
    ControllerStateDelta delta;
    delta.lastModule = event.module;
    delta.lastAction = event.action;
    delta.lastRequest = event.requestText;
    delta.operationActive = true;
    delta.operationProgress = -1.0;
    delta.operationMessage = requestMessage(event);
    delta.operationState = QStringLiteral("running");
    return delta;
}

auto OpenGeoLabFeedbackCoordinator::onProgress(const ProgressEvent& event) const
    -> ControllerStateDelta {
    ControllerStateDelta delta;
    delta.operationActive = event.active;
    delta.operationProgress = event.progress;
    delta.operationMessage = normalizedOperationMessage(event.message);
    delta.operationState = event.state;
    return delta;
}

auto OpenGeoLabFeedbackCoordinator::onRequestFinished(const RequestFinishedEvent& event) const
    -> ControllerStateDelta {
    ControllerStateDelta delta;
    const bool success = event.response.value("success", false);
    const auto payload = event.response.value("payload", nlohmann::json::object());
    const QString module = QString::fromStdString(event.response.value("module", std::string{}));
    const QString action = QString::fromStdString(event.response.value("action", std::string{}));
    const QString summary = QString::fromStdString(payload.value(
        "summary", event.response.value("message", std::string{"No summary available."})));
    const QString responseMessage =
        QString::fromStdString(event.response.value("message", std::string{}));

    delta.lastModule = module;
    delta.lastAction = action;
    delta.lastStatus = success ? QStringLiteral("Component request completed")
                               : QStringLiteral("Component request failed");
    delta.lastSummary = summary;
    delta.lastPayload = QString::fromStdString(buildUiResponsePreview(event.response).dump(2));
    delta.lastResponse = QString::fromStdString(buildPublicResponse(event.response).dump(2));
    delta.suggestedPython = QString::fromStdString(
        payload.value("exportedPython", payload.value("equivalentPython", std::string{})));
    delta.operationActive = false;
    delta.operationProgress = 1.0;
    delta.operationMessage = normalizedOperationMessage(summary);
    delta.operationState = success ? QStringLiteral("success") : QStringLiteral("error");
    delta.uiNotice = UiNotice{success ? 2 : 4, requestScope(module, action), summary,
                              secondaryOperationDetail(responseMessage, summary)};
    if(event.requestId != 0) {
        delta.serviceRequestFinished = std::make_pair(event.requestId, success);
    }
    return delta;
}

auto OpenGeoLabFeedbackCoordinator::onPythonOutput(const PythonOutputEvent& event) const
    -> ControllerStateDelta {
    ControllerStateDelta delta;
    delta.lastPythonOutput = event.outputText;
    return delta;
}

auto OpenGeoLabFeedbackCoordinator::onRecorderStateChanged(const RecorderStateEvent& event) const
    -> ControllerStateDelta {
    ControllerStateDelta delta;
    delta.recordedCommandCount = event.recordedCommandCount;
    delta.recordedCommands = event.recordedCommands;
    return delta;
}

auto OpenGeoLabFeedbackCoordinator::onUiNotice(const UiNotice& notice) const
    -> ControllerStateDelta {
    ControllerStateDelta delta;
    delta.lastStatus = QStringLiteral("UI notice reported");
    delta.lastSummary = normalizedOperationMessage(notice.message);
    delta.operationActive = false;
    delta.operationProgress = 1.0;
    delta.operationMessage = normalizedOperationMessage(notice.message);
    delta.operationState = notice.level >= 4 ? QStringLiteral("error") : QStringLiteral("success");
    delta.uiNotice = notice;
    return delta;
}

} // namespace OGL::App
