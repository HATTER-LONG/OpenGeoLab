#include <ogl/app/OpenGeoLabController.hpp>

#include <ogl/command/RequestProtocol.hpp>
#include <ogl/core/ModuleLogger.hpp>

#include "OpenGeoLabAutomationFacade.hpp"
#include "OpenGeoLabControllerEvents.hpp"
#include "OpenGeoLabFeedbackCoordinator.hpp"
#include "OpenGeoLabRequestExecutor.hpp"
#include "OperationLogModel.hpp"
#include "OperationLogService.hpp"
#include "QmlSpdlogSink.hpp"

#include <QDateTime>
#include <QThread>

namespace OGL::App {

namespace {

constexpr int kInfoOperationLevel = 2;
constexpr int kErrorOperationLevel = 4;

auto normalizedOperationScope(const QString& scope) -> QString {
    const QString trimmedScope = scope.trimmed();
    return trimmedScope.isEmpty() ? QStringLiteral("app") : trimmedScope;
}

auto extractModuleName(const nlohmann::json& requestJson) -> std::string {
    return requestJson.is_object() ? requestJson.value("module", std::string{}) : std::string{};
}

auto extractActionName(const nlohmann::json& requestJson) -> std::string {
    return requestJson.is_object() ? requestJson.value("action", std::string{}) : std::string{};
}

auto buildStartedEvent(const nlohmann::json& requestJson,
                       const QString& source,
                       bool isAsync,
                       int requestId = 0) -> RequestStartedEvent {
    return {.requestId = requestId,
            .source = source,
            .requestText = QString::fromStdString(requestJson.dump(2)),
            .module = QString::fromStdString(extractModuleName(requestJson)),
            .action = QString::fromStdString(extractActionName(requestJson)),
            .isAsync = isAsync};
}

} // namespace

OpenGeoLabController::OpenGeoLabController(QObject* parent)
    : QObject(parent), m_operationLogService(std::make_unique<OperationLogService>()),
      m_requestExecutor(std::make_unique<OpenGeoLabRequestExecutor>()),
      m_feedbackCoordinator(std::make_unique<OpenGeoLabFeedbackCoordinator>()),
      m_automationFacade(
          std::make_unique<OpenGeoLabAutomationFacade>([this](const nlohmann::json& requestJson) {
              const auto response =
                  executeRequestEnvelope(requestJson, QStringLiteral("embedded-python"), true);
              return response.value("success", false)
                         ? OpenGeoLabFeedbackCoordinator::buildPublicResponse(response)
                         : response;
          })) {
    QObject::connect(m_operationLogService.get(), &OperationLogService::hasNewErrorsChanged, this,
                     &OpenGeoLabController::hasUnreadOperationErrorsChanged);
    QObject::connect(m_operationLogService.get(), &OperationLogService::hasNewLogsChanged, this,
                     &OpenGeoLabController::hasUnreadOperationLogsChanged);
    OGL::Core::registerAdditionalLoggerSink(createQmlSpdlogSink(m_operationLogService.get()));
    resetOperationFeed();

    const auto snapshot = m_requestExecutor->recorderSnapshot();
    applyStateDelta(m_feedbackCoordinator->onRecorderStateChanged(
        {.recordedCommandCount = snapshot.recordedCommandCount,
         .recordedCommands = QString::fromStdString(snapshot.recordedCommands.dump(2))}));
}

OpenGeoLabController::~OpenGeoLabController() = default;

auto OpenGeoLabController::lastModule() const -> const QString& { return m_lastModule; }

auto OpenGeoLabController::lastAction() const -> const QString& { return m_lastAction; }

auto OpenGeoLabController::lastRequest() const -> const QString& { return m_lastRequest; }

auto OpenGeoLabController::lastStatus() const -> const QString& { return m_lastStatus; }

auto OpenGeoLabController::lastSummary() const -> const QString& { return m_lastSummary; }

auto OpenGeoLabController::lastPayload() const -> const QString& { return m_lastPayload; }

auto OpenGeoLabController::lastResponse() const -> const QString& { return m_lastResponse; }

auto OpenGeoLabController::suggestedPython() const -> const QString& { return m_suggestedPython; }

auto OpenGeoLabController::recordedCommandCount() const -> int { return m_recordedCommandCount; }

auto OpenGeoLabController::recordedCommands() const -> const QString& { return m_recordedCommands; }

auto OpenGeoLabController::lastPythonOutput() const -> const QString& { return m_lastPythonOutput; }

auto OpenGeoLabController::operationActive() const -> bool { return m_operationActive; }

auto OpenGeoLabController::operationProgress() const -> double { return m_operationProgress; }

auto OpenGeoLabController::operationMessage() const -> const QString& { return m_operationMessage; }

auto OpenGeoLabController::operationState() const -> const QString& { return m_operationState; }

auto OpenGeoLabController::operationLogModel() const -> QAbstractItemModel* {
    return m_operationLogService->model();
}

auto OpenGeoLabController::operationLogService() const -> QObject* {
    return m_operationLogService.get();
}

auto OpenGeoLabController::hasUnreadOperationErrors() const -> bool {
    return m_operationLogService->hasNewErrors();
}

auto OpenGeoLabController::hasUnreadOperationLogs() const -> bool {
    return m_operationLogService->hasNewLogs();
}

auto OpenGeoLabController::executeRequestEnvelope(const nlohmann::json& request_json,
                                                  const QString& source,
                                                  bool failIfAsyncActive) -> nlohmann::json {
    applyStateDelta(
        m_feedbackCoordinator->onRequestStarted(buildStartedEvent(request_json, source, false)));

    const auto eventSinks = OpenGeoLabRequestExecutor::EventSinks{
        .onProgress =
            [this](const ProgressEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onProgress(event));
            },
        .onRequestFinished =
            [this](const RequestFinishedEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onRequestFinished(event));
            },
        .onRecorderStateChanged =
            [this](const RecorderStateEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onRecorderStateChanged(event));
            }};

    try {
        auto commandRequest = OGL::Command::parseCommandRequest(request_json);
        return m_requestExecutor->executeSync(
            std::move(commandRequest), source, eventSinks,
            failIfAsyncActive ? OpenGeoLabRequestExecutor::BusyPolicy::FailIfAsyncActive
                              : OpenGeoLabRequestExecutor::BusyPolicy::WaitForAvailability);
    } catch(const std::exception& ex) {
        const auto response = OpenGeoLabRequestExecutor::buildFailureResponse(
            extractModuleName(request_json), extractActionName(request_json), ex.what());
        applyStateDelta(
            m_feedbackCoordinator->onRequestFinished({.requestId = 0, .response = response}));
        return response;
    }
}

auto OpenGeoLabController::executeCommand(const nlohmann::json& request_json,
                                          const std::string& source) -> nlohmann::json {
    const auto response =
        executeRequestEnvelope(request_json, QString::fromStdString(source), false);
    return response.value("success", false)
               ? OpenGeoLabFeedbackCoordinator::buildPublicResponse(response)
               : response;
}

auto OpenGeoLabController::replayRecordedCommandsJson() -> nlohmann::json {
    const nlohmann::json requestJson{{"module", "command"},
                                     {"action", "replayRecordedCommands"},
                                     {"param", nlohmann::json::object()}};
    applyStateDelta(m_feedbackCoordinator->onRequestStarted(
        buildStartedEvent(requestJson, QStringLiteral("command"), false)));

    const auto eventSinks = OpenGeoLabRequestExecutor::EventSinks{
        .onProgress =
            [this](const ProgressEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onProgress(event));
            },
        .onRequestFinished =
            [this](const RequestFinishedEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onRequestFinished(event));
            },
        .onRecorderStateChanged =
            [this](const RecorderStateEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onRecorderStateChanged(event));
            }};
    return m_requestExecutor->replayRecordedCommands(eventSinks);
}

auto OpenGeoLabController::clearRecordedCommandsJson() -> nlohmann::json {
    const nlohmann::json requestJson{{"module", "command"},
                                     {"action", "clearRecordedCommands"},
                                     {"param", nlohmann::json::object()}};
    applyStateDelta(m_feedbackCoordinator->onRequestStarted(
        buildStartedEvent(requestJson, QStringLiteral("command"), false)));

    const auto eventSinks = OpenGeoLabRequestExecutor::EventSinks{
        .onProgress =
            [this](const ProgressEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onProgress(event));
            },
        .onRequestFinished =
            [this](const RequestFinishedEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onRequestFinished(event));
            },
        .onRecorderStateChanged =
            [this](const RecorderStateEvent& event) {
                applyStateDelta(m_feedbackCoordinator->onRecorderStateChanged(event));
            }};
    return m_requestExecutor->clearRecordedCommands(eventSinks);
}

auto OpenGeoLabController::applicationStateJson() const -> nlohmann::json {
    nlohmann::json lastPayloadJson = nlohmann::json::object();
    if(!m_lastPayload.trimmed().isEmpty()) {
        try {
            lastPayloadJson = nlohmann::json::parse(m_lastPayload.toStdString());
        } catch(const std::exception&) {
            lastPayloadJson = nlohmann::json::object();
        }
    }

    const auto snapshot = m_requestExecutor->recorderSnapshot();
    return {{"lastModule", m_lastModule.toStdString()},
            {"lastAction", m_lastAction.toStdString()},
            {"lastRequestText", m_lastRequest.toStdString()},
            {"lastStatus", m_lastStatus.toStdString()},
            {"lastSummary", m_lastSummary.toStdString()},
            {"lastPayload", lastPayloadJson},
            {"lastPayloadText", m_lastPayload.toStdString()},
            {"suggestedPython", m_suggestedPython.toStdString()},
            {"lastPythonOutput", m_lastPythonOutput.toStdString()},
            {"recordedCommandCount", snapshot.recordedCommandCount},
            {"recordedCommands", snapshot.recordedCommands}};
}

bool OpenGeoLabController::runServiceRequest(const QString& request_json) {
    try {
        const nlohmann::json parsedRequest =
            request_json.trimmed().isEmpty() ? nlohmann::json::object()
                                             : nlohmann::json::parse(request_json.toStdString());
        return executeRequestEnvelope(parsedRequest, QStringLiteral("qml-ui"), false)
            .value("success", false);
    } catch(const std::exception& ex) {
        ControllerStateDelta delta;
        delta.lastRequest = request_json;
        applyStateDelta(delta);
        const auto response = OpenGeoLabRequestExecutor::buildFailureResponse(
            std::string{}, std::string{}, ex.what());
        applyStateDelta(
            m_feedbackCoordinator->onRequestFinished({.requestId = 0, .response = response}));
        return false;
    }
}

int OpenGeoLabController::submitServiceRequest(const QString& request_json) {
    if(m_requestExecutor->isAsyncRequestActive()) {
        applyStateDelta(m_feedbackCoordinator->onUiNotice(
            {.level = kErrorOperationLevel,
             .source = QStringLiteral("app.submitServiceRequest"),
             .message = QStringLiteral("Another service request is already running."),
             .detail = QString()}));
        return -1;
    }

    try {
        const nlohmann::json parsedRequest =
            request_json.trimmed().isEmpty() ? nlohmann::json::object()
                                             : nlohmann::json::parse(request_json.toStdString());
        applyStateDelta(m_feedbackCoordinator->onRequestStarted(
            buildStartedEvent(parsedRequest, QStringLiteral("qml-ui"), true)));
        auto commandRequest = OGL::Command::parseCommandRequest(parsedRequest);
        const auto eventSinks = OpenGeoLabRequestExecutor::EventSinks{
            .onProgress =
                [this](const ProgressEvent& event) {
                    applyStateDelta(m_feedbackCoordinator->onProgress(event));
                },
            .onRequestFinished =
                [this](const RequestFinishedEvent& event) {
                    applyStateDelta(m_feedbackCoordinator->onRequestFinished(event));
                },
            .onRecorderStateChanged =
                [this](const RecorderStateEvent& event) {
                    applyStateDelta(m_feedbackCoordinator->onRecorderStateChanged(event));
                }};
        const int requestId = m_requestExecutor->submitAsync(this, std::move(commandRequest),
                                                             QStringLiteral("qml-ui"), eventSinks);
        if(requestId < 0) {
            applyStateDelta(m_feedbackCoordinator->onUiNotice(
                {.level = kErrorOperationLevel,
                 .source = QStringLiteral("app.submitServiceRequest"),
                 .message = QStringLiteral("Another service request is already running."),
                 .detail = QString()}));
        }
        return requestId;
    } catch(const std::exception& ex) {
        ControllerStateDelta delta;
        delta.lastRequest = request_json;
        applyStateDelta(delta);
        const auto response = OpenGeoLabRequestExecutor::buildFailureResponse(
            std::string{}, std::string{}, ex.what());
        applyStateDelta(
            m_feedbackCoordinator->onRequestFinished({.requestId = -1, .response = response}));
        return -1;
    }
}

void OpenGeoLabController::markOperationLogSeen() { m_operationLogService->markAllSeen(); }

void OpenGeoLabController::clearOperationLog() { m_operationLogService->clear(); }

void OpenGeoLabController::replayRecordedCommands() { replayRecordedCommandsJson(); }

void OpenGeoLabController::clearRecordedCommands() { clearRecordedCommandsJson(); }

bool OpenGeoLabController::exportRecordedScript(const QString& file_path) {
    const auto snapshot = m_requestExecutor->recorderSnapshot();
    const auto result = m_automationFacade->exportRecordedScript(
        file_path, snapshot.exportedPython, m_suggestedPython, snapshot.recordedCommandCount);
    if(result.pythonOutput.has_value()) {
        applyStateDelta(m_feedbackCoordinator->onPythonOutput(*result.pythonOutput));
    }
    applyStateDelta(m_feedbackCoordinator->onUiNotice(result.notice));
    return result.success;
}

void OpenGeoLabController::runEmbeddedPython(const QString& script) {
    const auto result = m_automationFacade->runEmbeddedPython(script);
    if(result.pythonOutput.has_value()) {
        applyStateDelta(m_feedbackCoordinator->onPythonOutput(*result.pythonOutput));
    }
    applyStateDelta(m_feedbackCoordinator->onUiNotice(result.notice));
}

void OpenGeoLabController::runEmbeddedPythonCommandLine(const QString& command_line) {
    const auto result = m_automationFacade->runEmbeddedPythonCommandLine(command_line);
    if(result.pythonOutput.has_value()) {
        applyStateDelta(m_feedbackCoordinator->onPythonOutput(*result.pythonOutput));
    }
    applyStateDelta(m_feedbackCoordinator->onUiNotice(result.notice));
}

void OpenGeoLabController::postUiNotice(int level,
                                        const QString& source,
                                        const QString& message,
                                        const QString& detail) {
    applyStateDelta(m_feedbackCoordinator->onUiNotice(
        {.level = level, .source = source, .message = message, .detail = detail}));
}

void OpenGeoLabController::appendOperationLog(int level,
                                              const QString& source,
                                              const QString& message,
                                              const QString& detail,
                                              std::source_location location) {
    OperationLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = level;
    entry.levelName = operationLogLevelName(level);
    entry.source = normalizedOperationScope(source);
    entry.threadId = static_cast<qint64>(quintptr(QThread::currentThreadId()));
    entry.file = QString::fromUtf8(location.file_name());
    entry.line = static_cast<int>(location.line());
    entry.message = detail.trimmed().isEmpty()
                        ? message.trimmed()
                        : QStringLiteral("%1\n%2").arg(message.trimmed(), detail.trimmed());
    m_operationLogService->addEntry(std::move(entry));
}

void OpenGeoLabController::setOperationFeedback(bool active,
                                                double progress,
                                                const QString& message,
                                                const QString& state) {
    if(m_operationActive != active) {
        m_operationActive = active;
        emit operationActiveChanged();
    }

    if(m_operationProgress != progress) {
        m_operationProgress = progress;
        emit operationProgressChanged();
    }

    if(m_operationMessage != message) {
        m_operationMessage = message;
        emit operationMessageChanged();
    }

    if(m_operationState != state) {
        m_operationState = state;
        emit operationStateChanged();
    }
}

void OpenGeoLabController::resetOperationFeed() {
    m_operationLogService->clear();
    m_operationActive = false;
    m_operationProgress = 0.0;
    m_operationMessage.clear();
    m_operationState = QStringLiteral("idle");
}

void OpenGeoLabController::applyStateDelta(const ControllerStateDelta& delta) {
    if(delta.lastModule.has_value() && m_lastModule != *delta.lastModule) {
        m_lastModule = *delta.lastModule;
        emit lastModuleChanged();
    }
    if(delta.lastAction.has_value() && m_lastAction != *delta.lastAction) {
        m_lastAction = *delta.lastAction;
        emit lastActionChanged();
    }
    if(delta.lastRequest.has_value() && m_lastRequest != *delta.lastRequest) {
        m_lastRequest = *delta.lastRequest;
        emit lastRequestChanged();
    }
    if(delta.lastStatus.has_value() && m_lastStatus != *delta.lastStatus) {
        m_lastStatus = *delta.lastStatus;
        emit lastStatusChanged();
    }
    if(delta.lastSummary.has_value() && m_lastSummary != *delta.lastSummary) {
        m_lastSummary = *delta.lastSummary;
        emit lastSummaryChanged();
    }
    if(delta.lastPayload.has_value() && m_lastPayload != *delta.lastPayload) {
        m_lastPayload = *delta.lastPayload;
        emit lastPayloadChanged();
    }
    if(delta.lastResponse.has_value() && m_lastResponse != *delta.lastResponse) {
        m_lastResponse = *delta.lastResponse;
        emit lastResponseChanged();
    }
    if(delta.suggestedPython.has_value() && m_suggestedPython != *delta.suggestedPython) {
        m_suggestedPython = *delta.suggestedPython;
        emit suggestedPythonChanged();
    }
    if(delta.lastPythonOutput.has_value() && m_lastPythonOutput != *delta.lastPythonOutput) {
        m_lastPythonOutput = *delta.lastPythonOutput;
        emit lastPythonOutputChanged();
    }
    if(delta.recordedCommandCount.has_value() &&
       m_recordedCommandCount != *delta.recordedCommandCount) {
        m_recordedCommandCount = *delta.recordedCommandCount;
        emit recordedCommandCountChanged();
    }
    if(delta.recordedCommands.has_value() && m_recordedCommands != *delta.recordedCommands) {
        m_recordedCommands = *delta.recordedCommands;
        emit recordedCommandsChanged();
    }
    if(delta.operationActive.has_value()) {
        setOperationFeedback(*delta.operationActive,
                             delta.operationProgress.value_or(m_operationProgress),
                             delta.operationMessage.value_or(m_operationMessage),
                             delta.operationState.value_or(m_operationState));
    } else if(delta.operationProgress.has_value() || delta.operationMessage.has_value() ||
              delta.operationState.has_value()) {
        setOperationFeedback(m_operationActive,
                             delta.operationProgress.value_or(m_operationProgress),
                             delta.operationMessage.value_or(m_operationMessage),
                             delta.operationState.value_or(m_operationState));
    }
    if(delta.uiNotice.has_value()) {
        const UiNotice& notice = *delta.uiNotice;
        appendOperationLog(notice.level, notice.source, notice.message, notice.detail);
    }
    if(delta.serviceRequestFinished.has_value()) {
        emit serviceRequestFinished(delta.serviceRequestFinished->first,
                                    delta.serviceRequestFinished->second);
    }
}

} // namespace OGL::App
