#include <ogl/app/OpenGeoLabController.hpp>

#include <ogl/app/EmbeddedPythonRuntime.hpp>

#include <QSaveFile>
#include <QTextStream>
#include <QUrl>

#include <exception>
#include <nlohmann/json.hpp>

namespace OGL::App {

namespace {

auto defaultRecordSelectionRequest() -> nlohmann::json {
    return {{"operation", "pickPlaceholderEntity"},
            {"modelName", "RecordReplaySmokeModel"},
            {"bodyCount", 3},
            {"viewportWidth", 1280},
            {"viewportHeight", 720},
            {"screenX", 412},
            {"screenY", 248},
            {"requestedBy", "OpenGeoLabController::recordSelectionSmokeTest"}};
}

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

} // namespace

OpenGeoLabController::OpenGeoLabController(QObject* parent)
    : QObject(parent), m_commandRecorder(std::make_unique<OGL::Command::CommandRecorder>()),
      m_embeddedPythonRuntime(std::make_unique<EmbeddedPythonRuntime>(*this)) {
    runServiceRequest(QStringLiteral("selection"),
                      QStringLiteral("{\n"
                                     "  \"operation\": \"pickPlaceholderEntity\",\n"
                                     "  \"modelName\": \"Bracket_A01\",\n"
                                     "  \"bodyCount\": 3,\n"
                                     "  \"viewportWidth\": 1280,\n"
                                     "  \"viewportHeight\": 720,\n"
                                     "  \"screenX\": 412,\n"
                                     "  \"screenY\": 248,\n"
                                     "  \"source\": \"qml-ui\",\n"
                                     "  \"requestedBy\": \"OpenGeoLabController\"\n"
                                     "}"));
}

OpenGeoLabController::~OpenGeoLabController() = default;

auto OpenGeoLabController::lastModule() const -> const QString& { return m_lastModule; }

auto OpenGeoLabController::lastStatus() const -> const QString& { return m_lastStatus; }

auto OpenGeoLabController::lastSummary() const -> const QString& { return m_lastSummary; }

auto OpenGeoLabController::lastPayload() const -> const QString& { return m_lastPayload; }

auto OpenGeoLabController::suggestedPython() const -> const QString& { return m_suggestedPython; }

auto OpenGeoLabController::recordedCommandCount() const -> int {
    return m_commandRecorder->recordedCount();
}

auto OpenGeoLabController::recordedCommands() const -> const QString& { return m_recordedCommands; }

auto OpenGeoLabController::lastPythonOutput() const -> const QString& { return m_lastPythonOutput; }

auto OpenGeoLabController::executeCommand(const std::string& module_name,
                                          const nlohmann::json& request,
                                          const std::string& source) -> nlohmann::json {
    const QString trimmed_module = QString::fromStdString(module_name).trimmed();
    if(trimmed_module.isEmpty()) {
        const nlohmann::json response{{"success", false},
                                      {"message", "Module name cannot be empty."},
                                      {"payload", nlohmann::json::object()}};
        updateFromResponse(QString(), response);
        return response;
    }

    if(!request.is_object()) {
        const nlohmann::json response{{"success", false},
                                      {"message", "Request payload must be a JSON object."},
                                      {"payload", nlohmann::json::object()}};
        updateFromResponse(trimmed_module, response);
        return response;
    }

    nlohmann::json normalized_request = request;
    if(!source.empty() && !normalized_request.contains("source")) {
        normalized_request["source"] = source;
    }

    const OGL::Command::CommandRequest command_request{.moduleName = trimmed_module.toStdString(),
                                                       .params = std::move(normalized_request)};
    const auto response = buildAugmentedResponse(m_commandRecorder->execute(command_request));
    updateFromResponse(trimmed_module, response);
    return response;
}

auto OpenGeoLabController::replayRecordedCommandsJson() -> nlohmann::json {
    const auto replay_report = m_commandRecorder->replayAll();
    const bool success = replay_report.replayedCount == replay_report.successCount;

    const nlohmann::json response{
        {"success", success},
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
    updateFromResponse(QStringLiteral("command"), response);
    return response;
}

auto OpenGeoLabController::clearRecordedCommandsJson() -> nlohmann::json {
    m_commandRecorder->clear();
    const nlohmann::json response{{"success", true},
                                  {"message", "Recorded commands cleared."},
                                  {"payload",
                                   {{"summary", "Command history cleared."},
                                    {"exportedPython", m_commandRecorder->exportedPythonScript()},
                                    {"recordedCommands", m_commandRecorder->historyJson()},
                                    {"recordedCommandCount", m_commandRecorder->recordedCount()}}}};
    updateFromResponse(QStringLiteral("command"), response);
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

    return {{"lastModule", m_lastModule.toStdString()},
            {"lastStatus", m_lastStatus.toStdString()},
            {"lastSummary", m_lastSummary.toStdString()},
            {"lastPayload", last_payload_json},
            {"lastPayloadText", m_lastPayload.toStdString()},
            {"suggestedPython", m_suggestedPython.toStdString()},
            {"lastPythonOutput", m_lastPythonOutput.toStdString()},
            {"recordedCommandCount", m_commandRecorder->recordedCount()},
            {"recordedCommands", m_commandRecorder->historyJson()}};
}

void OpenGeoLabController::runServiceRequest(const QString& module_name,
                                             const QString& request_json) {
    try {
        nlohmann::json request = nlohmann::json::object();
        if(!request_json.trimmed().isEmpty()) {
            request = nlohmann::json::parse(request_json.toStdString());
        }
        executeCommand(module_name.toStdString(), request, "qml-ui");
    } catch(const std::exception& ex) {
        updateFromResponse(module_name.trimmed(),
                           nlohmann::json{{"success", false},
                                          {"message", ex.what()},
                                          {"payload", nlohmann::json::object()}});
    }
}

void OpenGeoLabController::replayRecordedCommands() { replayRecordedCommandsJson(); }

void OpenGeoLabController::clearRecordedCommands() { clearRecordedCommandsJson(); }

void OpenGeoLabController::recordSelectionSmokeTest() {
    clearRecordedCommandsJson();
    executeCommand("selection", defaultRecordSelectionRequest(), "qml-record-test");
}

bool OpenGeoLabController::exportRecordedScript(const QString& file_path) {
    const QString target_path = resolveExportPath(file_path).trimmed();
    const QString exported_script =
        m_suggestedPython.trimmed().isEmpty()
            ? QString::fromStdString(m_commandRecorder->exportedPythonScript())
            : m_suggestedPython;

    if(target_path.isEmpty()) {
        m_lastPythonOutput = QStringLiteral("Export path is empty.");
        emit lastPythonOutputChanged();
        return false;
    }

    if(exported_script.trimmed().isEmpty()) {
        m_lastPythonOutput = QStringLiteral("No recorded Python script is available to export.");
        emit lastPythonOutputChanged();
        return false;
    }

    QSaveFile output_file(target_path);
    if(!output_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastPythonOutput =
            QStringLiteral("Failed to open export file: %1").arg(output_file.errorString());
        emit lastPythonOutputChanged();
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
        return false;
    }

    m_lastStatus = QStringLiteral("Script export completed");
    m_lastSummary = QStringLiteral("Recorded Python script exported to %1").arg(target_path);
    m_lastPythonOutput = m_lastSummary;

    emit lastStatusChanged();
    emit lastSummaryChanged();
    emit lastPythonOutputChanged();
    return true;
}

void OpenGeoLabController::runEmbeddedPython(const QString& script) {
    if(script.trimmed().isEmpty()) {
        m_lastPythonOutput = QStringLiteral("Python script is empty.");
        emit lastPythonOutputChanged();
        return;
    }

    try {
        const std::string output = m_embeddedPythonRuntime->executeScript(script.toStdString());
        m_lastPythonOutput =
            formatPythonOutput(output, "Python script completed without stdout/stderr.");
    } catch(const std::exception& ex) {
        m_lastPythonOutput = QString::fromStdString(ex.what());
    }

    emit lastPythonOutputChanged();
}

void OpenGeoLabController::runEmbeddedPythonCommandLine(const QString& command_line) {
    if(command_line.trimmed().isEmpty()) {
        m_lastPythonOutput = QStringLiteral("Python command line is empty.");
        emit lastPythonOutputChanged();
        return;
    }

    try {
        const std::string output =
            m_embeddedPythonRuntime->executeCommandLine(command_line.toStdString());
        m_lastPythonOutput =
            formatPythonOutput(output, "Python command completed without stdout/stderr.");
    } catch(const std::exception& ex) {
        m_lastPythonOutput = QString::fromStdString(ex.what());
    }

    emit lastPythonOutputChanged();
}

auto OpenGeoLabController::buildAugmentedResponse(const OGL::Core::ServiceResponse& response) const
    -> nlohmann::json {
    nlohmann::json response_json = response.toJson();
    response_json["payload"]["recordedCommands"] = m_commandRecorder->historyJson();
    response_json["payload"]["recordedCommandCount"] = m_commandRecorder->recordedCount();
    response_json["payload"]["exportedPython"] = m_commandRecorder->exportedPythonScript();
    return response_json;
}

void OpenGeoLabController::updateRecorderState() {
    m_recordedCommands = QString::fromStdString(m_commandRecorder->historyJson().dump(2));
    emit recordedCommandCountChanged();
    emit recordedCommandsChanged();
}

void OpenGeoLabController::updateFromResponse(const QString& module_name,
                                              const nlohmann::json& response) {
    const bool success = response.value("success", false);
    const nlohmann::json payload = response.value("payload", nlohmann::json::object());

    m_lastModule = module_name;
    m_lastStatus = QString::fromStdString(success ? "Component request completed"
                                                  : "Component request failed");
    m_lastSummary = QString::fromStdString(
        payload.value("summary", response.value("message", std::string{"No summary available."})));
    m_lastPayload = QString::fromStdString(response.dump(2));
    m_suggestedPython = QString::fromStdString(
        payload.value("exportedPython", payload.value("equivalentPython", std::string{})));

    emit lastModuleChanged();
    emit lastStatusChanged();
    emit lastSummaryChanged();
    emit lastPayloadChanged();
    emit suggestedPythonChanged();
    updateRecorderState();
}

} // namespace OGL::App