/**
 * @file OpenGeoLabControllerEvents.hpp
 * @brief Internal event and delta types used to split controller execution and feedback concerns.
 */

#pragma once

#include <QString>

#include <nlohmann/json.hpp>

#include <optional>
#include <utility>

namespace OGL::App {

struct UiNotice {
    int level{0};
    QString source;
    QString message;
    QString detail;
};

struct ControllerStateDelta {
    std::optional<QString> lastModule;
    std::optional<QString> lastAction;
    std::optional<QString> lastRequest;
    std::optional<QString> lastStatus;
    std::optional<QString> lastSummary;
    std::optional<QString> lastPayload;
    std::optional<QString> lastResponse;
    std::optional<QString> suggestedPython;
    std::optional<QString> lastPythonOutput;
    std::optional<int> recordedCommandCount;
    std::optional<QString> recordedCommands;
    std::optional<bool> operationActive;
    std::optional<double> operationProgress;
    std::optional<QString> operationMessage;
    std::optional<QString> operationState;
    std::optional<std::pair<int, bool>> serviceRequestFinished;
    std::optional<UiNotice> uiNotice;
};

struct RequestStartedEvent {
    int requestId{0};
    QString source;
    QString requestText;
    QString module;
    QString action;
    bool isAsync{false};
};

struct ProgressEvent {
    int requestId{0};
    bool active{true};
    double progress{0.0};
    QString message;
    QString state;
};

struct RequestFinishedEvent {
    int requestId{0};
    nlohmann::json response = nlohmann::json::object();
};

struct PythonOutputEvent {
    QString outputText;
    bool treatAsProcessResponse{false};
};

struct RecorderStateEvent {
    int recordedCommandCount{0};
    QString recordedCommands;
};

} // namespace OGL::App
