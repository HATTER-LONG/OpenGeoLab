#include <ogl/app/OpenGeoLabController.hpp>

#include <ogl/app/EmbeddedPythonRuntime.hpp>
#include <ogl/core/ModuleLogger.hpp>

#include "OperationLogModel.hpp"
#include "OperationLogService.hpp"
#include "QmlSpdlogSink.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QFutureWatcher>
#include <QPointer>
#include <QSaveFile>
#include <QTextStream>
#include <QThread>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <exception>
#include <stdexcept>

namespace OGL::App {

namespace {

constexpr int kInfoOperationLevel = 2;
constexpr int kErrorOperationLevel = 4;

struct AsyncServiceRequestResult {
    int requestId{0};
    nlohmann::json response = nlohmann::json::object();
};

auto formatPythonOutput(const std::string& output, const char* empty_message) -> QString {
    return QString::fromStdString(output.empty() ? std::string{empty_message} : output);
}

auto resolveExportPath(const QString& file_path) -> QString {
    const QUrl url(file_path);
    if(url.isLocalFile()) {
        return url.toLocalFile();
    }

    return file_path;
}

auto normalizedOperationScope(const QString& scope) -> QString {
    const QString trimmed_scope = scope.trimmed();
    return trimmed_scope.isEmpty() ? QStringLiteral("app") : trimmed_scope;
}

auto normalizedOperationMessage(const QString& message) -> QString {
    const QString trimmed_message = message.trimmed();
    return trimmed_message.isEmpty() ? QStringLiteral("Operation updated.") : trimmed_message;
}

auto secondaryOperationDetail(const QString& detail, const QString& primary_message) -> QString {
    const QString trimmed_detail = detail.trimmed();
    if(trimmed_detail.isEmpty() || trimmed_detail == primary_message.trimmed()) {
        return QString{};
    }
    return trimmed_detail;
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

auto requestScope(const std::string& module, const std::string& action) -> QString {
    return requestScope(QString::fromStdString(module), QString::fromStdString(action));
}

void copyIfPresent(const nlohmann::json& source,
                   nlohmann::json& target,
                   const char* source_key,
                   const char* target_key = nullptr) {
    if(source.contains(source_key)) {
        target[target_key == nullptr ? source_key : target_key] = source.at(source_key);
    }
}

struct JsonFieldSpec {
    const char* sourceKey;
    const char* targetKey{nullptr};
};

auto buildObjectSubset(const nlohmann::json& source,
                       std::initializer_list<JsonFieldSpec> fields) -> nlohmann::json {
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
                          const char* payload_key,
                          std::initializer_list<JsonFieldSpec> fields) {
    const auto subset = buildObjectSubset(payload.value(payload_key, nlohmann::json::object()), fields);
    for(const auto& [key, value] : subset.items()) {
        target[key] = value;
    }
}

void copyNestedObjectSubset(const nlohmann::json& payload,
                            nlohmann::json& target,
                            const char* payload_key,
                            std::initializer_list<JsonFieldSpec> fields) {
    auto subset = buildObjectSubset(payload.value(payload_key, nlohmann::json::object()), fields);
    if(!subset.empty()) {
        target[payload_key] = std::move(subset);
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

auto buildPublicResponse(nlohmann::json response) -> nlohmann::json {
    const auto success = response.value("success", false);
    const auto module = response.value("module", std::string{});
    const auto action = response.value("action", std::string{});
    const auto payload = response.value("payload", nlohmann::json::object());
    nlohmann::json public_payload = nlohmann::json::object();
    copyIfPresent(payload, public_payload, "shapeType");
    copyIfPresent(payload, public_payload, "recordedCommandCount");
    copyNestedObjectSubset(payload, public_payload, "model",
                           {{"modelName"}, {"bodyCount"}, {"source"}});
    copyNestedObjectSubset(payload, public_payload, "geometryModel",
                           {{"modelName"}, {"bodyCount"}, {"source"}});
    copyNestedObjectSubset(payload, public_payload, "sceneGraph", {{"sceneId"}, {"nodeCount"}});
    copyNestedObjectSubset(payload, public_payload, "renderFrame",
                           {{"frameId"}, {"drawItemCount"}});
    copyNestedObjectSubset(payload, public_payload, "selectionResult",
                           {{"mode"}, {"hitCount"}});
    copyNestedObjectSubset(payload, public_payload, "replayReport",
                           {{"replayedCount"}, {"successCount"}});

    const std::string public_summary =
        !module.empty() && !action.empty()
            ? (module + " " + action +
               (success ? " completed successfully." : " failed to complete."))
            : response.value("message", std::string{"Request completed."});

    return {{"success", success},
            {"summary", public_summary},
            {"payload", std::move(public_payload)}};
}

auto requestMessage(const OGL::Command::CommandRequest& request) -> QString {
    return QStringLiteral("Running %1...").arg(requestScope(request.module, request.action));
}

auto parseCommandRequest(const nlohmann::json& request_json) -> OGL::Command::CommandRequest {
    if(!request_json.is_object()) {
        throw std::invalid_argument("Request payload must be a JSON object.");
    }

    const std::string module = request_json.value("module", std::string{});
    if(module.empty()) {
        throw std::invalid_argument("Request module cannot be empty.");
    }

    const std::string action = request_json.value("action", std::string{});
    if(action.empty()) {
        throw std::invalid_argument("Request action cannot be empty.");
    }

    nlohmann::json param =
        request_json.contains("param") ? request_json.at("param") : nlohmann::json::object();
    if(param.is_null()) {
        param = nlohmann::json::object();
    }
    if(!param.is_object()) {
        throw std::invalid_argument("Request param must be a JSON object.");
    }

    return {.module = module, .action = action, .param = std::move(param)};
}

auto failureResponse(const std::string& module,
                     const std::string& action,
                     const std::string& message) -> nlohmann::json {
    return {{"success", false},
            {"module", module},
            {"action", action},
            {"message", message},
            {"payload", nlohmann::json::object()}};
}

auto extractModuleName(const nlohmann::json& request_json) -> std::string {
    return request_json.is_object() ? request_json.value("module", std::string{}) : std::string{};
}

auto extractActionName(const nlohmann::json& request_json) -> std::string {
    return request_json.is_object() ? request_json.value("action", std::string{}) : std::string{};
}

} // namespace

OpenGeoLabController::OpenGeoLabController(QObject* parent)
    : QObject(parent), m_operationLogService(std::make_unique<OperationLogService>()),
      m_commandRecorder(std::make_unique<OGL::Command::CommandRecorder>()),
      m_embeddedPythonRuntime(std::make_unique<EmbeddedPythonRuntime>(*this)) {
    QObject::connect(m_operationLogService.get(), &OperationLogService::hasNewErrorsChanged, this,
                     &OpenGeoLabController::hasUnreadOperationErrorsChanged);
    QObject::connect(m_operationLogService.get(), &OperationLogService::hasNewLogsChanged, this,
                     &OpenGeoLabController::hasUnreadOperationLogsChanged);
    OGL::Core::registerAdditionalLoggerSink(createQmlSpdlogSink(m_operationLogService.get()));
    updateRecorderState();
    resetOperationFeed();
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

auto OpenGeoLabController::executeCommand(const nlohmann::json& request_json,
                                          const std::string& source) -> nlohmann::json {
    try {
        return executeParsedCommand(parseCommandRequest(request_json), source);
    } catch(const std::exception& ex) {
        const auto response = failureResponse(extractModuleName(request_json),
                                              extractActionName(request_json), ex.what());
        updateFromResponse(response);
        return response;
    }
}

auto OpenGeoLabController::executeParsedCommand(OGL::Command::CommandRequest request,
                                                const std::string& source) -> nlohmann::json {
    setLastRequestText(request.toJson());
    const auto progress_callback = [this](double progress, const std::string& message) {
        setOperationFeedback(true, std::clamp(progress, 0.0, 1.0),
                             normalizedOperationMessage(QString::fromStdString(message)),
                             QStringLiteral("running"));
        // Requests still execute synchronously on the UI thread in the direct execution path.
        // Processing pending events lets the overlay repaint while action code simulates work.
        QCoreApplication::processEvents();
        return true;
    };
    const auto response = performCommandRequest(std::move(request), source, progress_callback);
    updateFromResponse(response);
    return buildPublicResponse(response);
}

auto OpenGeoLabController::performCommandRequest(
    OGL::Command::CommandRequest request,
    const std::string& source,
    const OGL::Core::ProgressCallback& progress_callback) -> nlohmann::json {
    if(!source.empty() && !request.param.contains("source")) {
        request.param["source"] = source;
    }

    std::scoped_lock lock(m_commandExecutionMutex);
    const auto response = m_commandRecorder->execute(request, progress_callback);
    return response.toJson();
}

auto OpenGeoLabController::replayRecordedCommandsJson() -> nlohmann::json {
    setLastRequestText(nlohmann::json{{"module", "command"},
                                      {"action", "replayRecordedCommands"},
                                      {"param", nlohmann::json::object()}});
    beginOperation(QStringLiteral("command.replayRecordedCommands"),
                   QStringLiteral("Replaying recorded commands..."));
    nlohmann::json response;
    {
        std::scoped_lock lock(m_commandExecutionMutex);
        const auto replay_report = m_commandRecorder->replayAll();
        const bool success = replay_report.replayedCount == replay_report.successCount;

        response = {{"success", success},
                    {"module", "command"},
                    {"action", "replayRecordedCommands"},
                    {"message", success ? "Recorded commands replayed successfully."
                                        : "Recorded command replay finished with failures."},
                    {"payload",
                     {{"summary", QStringLiteral("Replayed %1 command(s); %2 succeeded.")
                                      .arg(replay_report.replayedCount)
                                      .arg(replay_report.successCount)
                                      .toStdString()},
                      {"replayReport", replay_report.toJson()},
                      {"exportedPython", m_commandRecorder->exportedPythonScript()},
                      {"recordedCommands", m_commandRecorder->historyJson()},
                      {"recordedCommandCount", m_commandRecorder->recordedCount()}}}};
    }
    updateFromResponse(response);
    return response;
}

auto OpenGeoLabController::clearRecordedCommandsJson() -> nlohmann::json {
    setLastRequestText(nlohmann::json{{"module", "command"},
                                      {"action", "clearRecordedCommands"},
                                      {"param", nlohmann::json::object()}});
    beginOperation(QStringLiteral("command.clearRecordedCommands"),
                   QStringLiteral("Clearing recorded command history..."));
    nlohmann::json response;
    {
        std::scoped_lock lock(m_commandExecutionMutex);
        m_commandRecorder->clear();
        response = {{"success", true},
                    {"module", "command"},
                    {"action", "clearRecordedCommands"},
                    {"message", "Recorded commands cleared."},
                    {"payload",
                     {{"summary", "Command history cleared."},
                      {"exportedPython", m_commandRecorder->exportedPythonScript()},
                      {"recordedCommands", m_commandRecorder->historyJson()},
                      {"recordedCommandCount", m_commandRecorder->recordedCount()}}}};
    }
    updateFromResponse(response);
    return response;
}

auto OpenGeoLabController::applicationStateJson() const -> nlohmann::json {
    nlohmann::json last_payload_json = nlohmann::json::object();
    if(!m_lastPayload.trimmed().isEmpty()) {
        try {
            last_payload_json = nlohmann::json::parse(m_lastPayload.toStdString());
        } catch(const std::exception&) {
            last_payload_json = nlohmann::json::object();
        }
    }

    std::scoped_lock lock(m_commandExecutionMutex);
    return {{"lastModule", m_lastModule.toStdString()},
            {"lastAction", m_lastAction.toStdString()},
            {"lastRequestText", m_lastRequest.toStdString()},
            {"lastStatus", m_lastStatus.toStdString()},
            {"lastSummary", m_lastSummary.toStdString()},
            {"lastPayload", last_payload_json},
            {"lastPayloadText", m_lastPayload.toStdString()},
            {"suggestedPython", m_suggestedPython.toStdString()},
            {"lastPythonOutput", m_lastPythonOutput.toStdString()},
            {"recordedCommandCount", m_commandRecorder->recordedCount()},
            {"recordedCommands", m_commandRecorder->historyJson()}};
}

bool OpenGeoLabController::runServiceRequest(const QString& request_json) {
    try {
        const nlohmann::json parsed_request =
            request_json.trimmed().isEmpty() ? nlohmann::json::object()
                                             : nlohmann::json::parse(request_json.toStdString());
        auto command_request = parseCommandRequest(parsed_request);
        beginOperation(requestScope(command_request.module, command_request.action),
                       requestMessage(command_request));
        return executeParsedCommand(std::move(command_request), "qml-ui").value("success", false);
    } catch(const std::exception& ex) {
        const auto response = failureResponse(std::string{}, std::string{}, ex.what());
        updateFromResponse(response);
        return false;
    }
}

int OpenGeoLabController::submitServiceRequest(const QString& request_json) {
    try {
        if(m_asyncServiceRequestActive) {
            completeOperation(false, QStringLiteral("app.submitServiceRequest"),
                              QStringLiteral("Another service request is already running."));
            return -1;
        }

        const nlohmann::json parsed_request =
            request_json.trimmed().isEmpty() ? nlohmann::json::object()
                                             : nlohmann::json::parse(request_json.toStdString());
        auto command_request = parseCommandRequest(parsed_request);
        const int request_id = m_nextAsyncRequestId++;
        setLastRequestText(command_request.toJson());
        beginOperation(requestScope(command_request.module, command_request.action),
                       requestMessage(command_request));
        m_asyncServiceRequestActive = true;

        auto* watcher = new QFutureWatcher<AsyncServiceRequestResult>(this);
        QObject::connect(watcher, &QFutureWatcher<AsyncServiceRequestResult>::finished, this,
                         [this, watcher]() {
                             const AsyncServiceRequestResult result = watcher->result();
                             watcher->deleteLater();
                             m_asyncServiceRequestActive = false;
                             updateFromResponse(result.response);
                             emit serviceRequestFinished(result.requestId,
                                                         result.response.value("success", false));
                         });

        QPointer<OpenGeoLabController> self(this);
        watcher->setFuture(QtConcurrent::run([self, request_id,
                                              request = std::move(command_request)]() mutable {
            if(!self) {
                return AsyncServiceRequestResult{
                    .requestId = request_id,
                    .response =
                        failureResponse(request.module, request.action,
                                        "Controller was destroyed before the request completed."),
                };
            }

            const auto progress_callback = [self](double progress, const std::string& message) {
                if(!self) {
                    return false;
                }

                QMetaObject::invokeMethod(
                    self,
                    [self, progress, message]() {
                        if(!self) {
                            return;
                        }
                        self->setOperationFeedback(
                            true, std::clamp(progress, 0.0, 1.0),
                            normalizedOperationMessage(QString::fromStdString(message)),
                            QStringLiteral("running"));
                    },
                    Qt::BlockingQueuedConnection);
                return true;
            };

            return AsyncServiceRequestResult{
                .requestId = request_id,
                .response =
                    self->performCommandRequest(std::move(request), "qml-ui", progress_callback),
            };
        }));
        return request_id;
    } catch(const std::exception& ex) {
        const auto response = failureResponse(std::string{}, std::string{}, ex.what());
        updateFromResponse(response);
        emit serviceRequestFinished(-1, false);
        return -1;
    }
}

void OpenGeoLabController::markOperationLogSeen() { m_operationLogService->markAllSeen(); }

void OpenGeoLabController::clearOperationLog() { m_operationLogService->clear(); }

void OpenGeoLabController::replayRecordedCommands() { replayRecordedCommandsJson(); }

void OpenGeoLabController::clearRecordedCommands() { clearRecordedCommandsJson(); }

bool OpenGeoLabController::exportRecordedScript(const QString& file_path) {
    beginOperation(QStringLiteral("python.exportRecordedScript"),
                   QStringLiteral("Exporting recorded Python script..."));
    const QString target_path = resolveExportPath(file_path).trimmed();
    QString recorded_script;
    QString exported_script;
    {
        std::scoped_lock lock(m_commandExecutionMutex);
        recorded_script = QString::fromStdString(m_commandRecorder->exportedPythonScript());
        exported_script =
            m_commandRecorder->recordedCount() > 0
                ? recorded_script
                : (m_suggestedPython.trimmed().isEmpty() ? recorded_script : m_suggestedPython);
    }

    if(target_path.isEmpty()) {
        m_lastPythonOutput = QStringLiteral("Export path is empty.");
        emit lastPythonOutputChanged();
        completeOperation(false, QStringLiteral("python.exportRecordedScript"), m_lastPythonOutput);
        return false;
    }

    if(exported_script.trimmed().isEmpty()) {
        m_lastPythonOutput = QStringLiteral("No recorded Python script is available to export.");
        emit lastPythonOutputChanged();
        completeOperation(false, QStringLiteral("python.exportRecordedScript"), m_lastPythonOutput);
        return false;
    }

    QSaveFile output_file(target_path);
    if(!output_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastPythonOutput =
            QStringLiteral("Failed to open export file: %1").arg(output_file.errorString());
        emit lastPythonOutputChanged();
        completeOperation(false, QStringLiteral("python.exportRecordedScript"), m_lastPythonOutput);
        return false;
    }

    QTextStream output_stream(&output_file);
    output_stream << exported_script;
    if(!exported_script.endsWith('\n')) {
        output_stream << '\n';
    }

    if(!output_file.commit()) {
        m_lastPythonOutput =
            QStringLiteral("Failed to save export file: %1").arg(output_file.errorString());
        emit lastPythonOutputChanged();
        completeOperation(false, QStringLiteral("python.exportRecordedScript"), m_lastPythonOutput);
        return false;
    }

    m_lastStatus = QStringLiteral("Script export completed");
    m_lastSummary = QStringLiteral("Recorded Python script exported to %1").arg(target_path);
    m_lastPythonOutput = m_lastSummary;

    emit lastStatusChanged();
    emit lastSummaryChanged();
    emit lastPythonOutputChanged();
    completeOperation(true, QStringLiteral("python.exportRecordedScript"), m_lastSummary);
    return true;
}

void OpenGeoLabController::runEmbeddedPython(const QString& script) {
    beginOperation(QStringLiteral("python.executeScript"),
                   QStringLiteral("Running embedded Python script..."));
    if(script.trimmed().isEmpty()) {
        m_lastPythonOutput = QStringLiteral("Python script is empty.");
        emit lastPythonOutputChanged();
        completeOperation(false, QStringLiteral("python.executeScript"), m_lastPythonOutput);
        return;
    }

    try {
        const std::string output = m_embeddedPythonRuntime->executeScript(script.toStdString());
        m_lastPythonOutput =
            formatPythonOutput(output, "Python script completed without stdout/stderr.");
        completeOperation(true, QStringLiteral("python.executeScript"),
                          QStringLiteral("Embedded Python script completed."), m_lastPythonOutput);
    } catch(const std::exception& ex) {
        m_lastPythonOutput = QString::fromStdString(ex.what());
        completeOperation(false, QStringLiteral("python.executeScript"),
                          QStringLiteral("Embedded Python script failed."), m_lastPythonOutput);
    }

    emit lastPythonOutputChanged();
}

void OpenGeoLabController::runEmbeddedPythonCommandLine(const QString& command_line) {
    beginOperation(QStringLiteral("python.commandLine"),
                   QStringLiteral("Running Python command line..."));
    if(command_line.trimmed().isEmpty()) {
        m_lastPythonOutput = QStringLiteral("Python command line is empty.");
        emit lastPythonOutputChanged();
        completeOperation(false, QStringLiteral("python.commandLine"), m_lastPythonOutput);
        return;
    }

    try {
        const std::string output =
            m_embeddedPythonRuntime->executeCommandLine(command_line.toStdString());
        m_lastPythonOutput =
            formatPythonOutput(output, "Python command completed without stdout/stderr.");
        completeOperation(true, QStringLiteral("python.commandLine"),
                          QStringLiteral("Python command line completed."), m_lastPythonOutput);
    } catch(const std::exception& ex) {
        m_lastPythonOutput = QString::fromStdString(ex.what());
        completeOperation(false, QStringLiteral("python.commandLine"),
                          QStringLiteral("Python command line failed."), m_lastPythonOutput);
    }

    emit lastPythonOutputChanged();
}

void OpenGeoLabController::beginOperation(const QString& scope, const QString& message) {
    if(m_suppressUiActivity) {
        return;
    }

    setOperationFeedback(true, -1.0, normalizedOperationMessage(message),
                         QStringLiteral("running"));
}

void OpenGeoLabController::completeOperation(bool success,
                                             const QString& scope,
                                             const QString& message,
                                             const QString& detail) {
    if(m_suppressUiActivity) {
        return;
    }

    const QString normalized_message = normalizedOperationMessage(message);
    appendOperationLog(success ? kInfoOperationLevel : kErrorOperationLevel, scope,
                       normalized_message, detail);
    setOperationFeedback(false, 1.0, normalized_message,
                         success ? QStringLiteral("success") : QStringLiteral("error"));
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
    entry.message =
        detail.trimmed().isEmpty()
            ? normalizedOperationMessage(message)
            : QStringLiteral("%1\n%2").arg(normalizedOperationMessage(message), detail.trimmed());
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

void OpenGeoLabController::updateRecorderState() {
    QString next_recorded_commands;
    int next_recorded_command_count = 0;
    {
        std::scoped_lock lock(m_commandExecutionMutex);
        next_recorded_commands = QString::fromStdString(m_commandRecorder->historyJson().dump(2));
        next_recorded_command_count = m_commandRecorder->recordedCount();
    }

    if(m_recordedCommandCount != next_recorded_command_count) {
        m_recordedCommandCount = next_recorded_command_count;
        emit recordedCommandCountChanged();
    }

    if(m_recordedCommands != next_recorded_commands) {
        m_recordedCommands = std::move(next_recorded_commands);
        emit recordedCommandsChanged();
    }
}

void OpenGeoLabController::setLastRequestText(const nlohmann::json& request_json) {
    const QString next_request = QString::fromStdString(request_json.dump(2));
    if(m_lastRequest == next_request) {
        return;
    }
    m_lastRequest = next_request;
    emit lastRequestChanged();
}

void OpenGeoLabController::updateFromResponse(const nlohmann::json& response) {
    const bool success = response.value("success", false);
    const nlohmann::json payload = response.value("payload", nlohmann::json::object());
    const nlohmann::json ui_preview = buildUiResponsePreview(response);

    m_lastModule = QString::fromStdString(response.value("module", std::string{}));
    m_lastAction = QString::fromStdString(response.value("action", std::string{}));
    m_lastStatus = QString::fromStdString(success ? "Component request completed"
                                                  : "Component request failed");
    m_lastSummary = QString::fromStdString(
        payload.value("summary", response.value("message", std::string{"No summary available."})));
    m_lastPayload = QString::fromStdString(ui_preview.dump(2));
    m_lastResponse = QString::fromStdString(buildPublicResponse(response).dump(2));
    m_suggestedPython = QString::fromStdString(
        payload.value("exportedPython", payload.value("equivalentPython", std::string{})));

    emit lastModuleChanged();
    emit lastActionChanged();
    emit lastStatusChanged();
    emit lastSummaryChanged();
    emit lastPayloadChanged();
    emit lastResponseChanged();
    emit suggestedPythonChanged();
    updateRecorderState();

    const QString response_message =
        QString::fromStdString(response.value("message", std::string{}));
    const QString operation_scope = requestScope(m_lastModule, m_lastAction);
    const QString operation_detail = secondaryOperationDetail(response_message, m_lastSummary);
    if(success) {
        setOperationFeedback(false, 1.0, normalizedOperationMessage(m_lastSummary),
                             QStringLiteral("success"));
        return;
    }

    completeOperation(false, operation_scope, m_lastSummary, operation_detail);
}

} // namespace OGL::App
