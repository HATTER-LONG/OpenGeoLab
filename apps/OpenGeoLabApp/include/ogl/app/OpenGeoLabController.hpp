/**
 * @file OpenGeoLabController.hpp
 * @brief QML-facing controller that drives the placeholder geometry pipeline.
 */

#pragma once

#include <QObject>
#include <QString>

#include <ogl/command/CommandService.hpp>

#include <nlohmann/json.hpp>

#include <memory>

namespace OGL::App {

class EmbeddedPythonRuntime;

/**
 * @brief Thin application controller exposing command and automation entry points to QML.
 */
class OpenGeoLabController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastModule READ lastModule NOTIFY lastModuleChanged)
    Q_PROPERTY(QString lastStatus READ lastStatus NOTIFY lastStatusChanged)
    Q_PROPERTY(QString lastSummary READ lastSummary NOTIFY lastSummaryChanged)
    Q_PROPERTY(QString lastPayload READ lastPayload NOTIFY lastPayloadChanged)
    Q_PROPERTY(QString suggestedPython READ suggestedPython NOTIFY suggestedPythonChanged)
    Q_PROPERTY(
        int recordedCommandCount READ recordedCommandCount NOTIFY recordedCommandCountChanged)
    Q_PROPERTY(QString recordedCommands READ recordedCommands NOTIFY recordedCommandsChanged)
    Q_PROPERTY(QString lastPythonOutput READ lastPythonOutput NOTIFY lastPythonOutputChanged)

public:
    /**
     * @brief Construct the UI-facing controller.
     * @param parent Optional QObject parent.
     */
    explicit OpenGeoLabController(QObject* parent = nullptr);
    ~OpenGeoLabController() override;

    [[nodiscard]] auto lastModule() const -> const QString&;
    [[nodiscard]] auto lastStatus() const -> const QString&;
    [[nodiscard]] auto lastSummary() const -> const QString&;
    [[nodiscard]] auto lastPayload() const -> const QString&;
    [[nodiscard]] auto suggestedPython() const -> const QString&;
    [[nodiscard]] auto recordedCommandCount() const -> int;
    [[nodiscard]] auto recordedCommands() const -> const QString&;
    [[nodiscard]] auto lastPythonOutput() const -> const QString&;

    auto executeCommand(const std::string& module_name,
                        const nlohmann::json& request,
                        const std::string& source) -> nlohmann::json;
    auto replayRecordedCommandsJson() -> nlohmann::json;
    auto clearRecordedCommandsJson() -> nlohmann::json;
    [[nodiscard]] auto applicationStateJson() const -> nlohmann::json;

    /**
     * @brief Execute a generic service request chosen by QML through module name and JSON payload.
     * @param module_name Target module name resolved through Kangaroo factories.
     * @param request_json JSON payload string passed from QML.
     */
    Q_INVOKABLE void runServiceRequest(const QString& module_name, const QString& request_json);
    Q_INVOKABLE void replayRecordedCommands();
    Q_INVOKABLE void clearRecordedCommands();
    Q_INVOKABLE void recordSelectionSmokeTest();
    Q_INVOKABLE void runEmbeddedPython(const QString& script);
    Q_INVOKABLE void runEmbeddedPythonCommandLine(const QString& command_line);

signals:
    void lastModuleChanged();
    void lastStatusChanged();
    void lastSummaryChanged();
    void lastPayloadChanged();
    void suggestedPythonChanged();
    void recordedCommandCountChanged();
    void recordedCommandsChanged();
    void lastPythonOutputChanged();

private:
    void updateFromResponse(const QString& module_name, const nlohmann::json& response);
    [[nodiscard]] auto buildAugmentedResponse(const OGL::Core::ServiceResponse& response) const
        -> nlohmann::json;
    void updateRecorderState();

    std::unique_ptr<OGL::Command::CommandRecorder> m_commandRecorder;
    std::unique_ptr<EmbeddedPythonRuntime> m_embeddedPythonRuntime;
    QString m_lastModule;
    QString m_lastStatus;
    QString m_lastSummary;
    QString m_lastPayload;
    QString m_suggestedPython;
    QString m_recordedCommands;
    QString m_lastPythonOutput;
};

} // namespace OGL::App