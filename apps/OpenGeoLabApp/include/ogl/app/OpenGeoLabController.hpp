/**
 * @file OpenGeoLabController.hpp
 * @brief QML-facing controller that bridges generic module/action/param requests and automation
 * tools.
 */

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

#include <nlohmann/json.hpp>

#include <memory>
#include <source_location>
#include <string>

namespace OGL::App {

class OperationLogService;
class OpenGeoLabRequestExecutor;
class OpenGeoLabFeedbackCoordinator;
class OpenGeoLabAutomationFacade;
struct ControllerStateDelta;

/**
 * @brief Thin application controller exposing generic protocol requests to QML and Python.
 */
class OpenGeoLabController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastModule READ lastModule NOTIFY lastModuleChanged)
    Q_PROPERTY(QString lastAction READ lastAction NOTIFY lastActionChanged)
    Q_PROPERTY(QString lastRequest READ lastRequest NOTIFY lastRequestChanged)
    Q_PROPERTY(QString lastStatus READ lastStatus NOTIFY lastStatusChanged)
    Q_PROPERTY(QString lastSummary READ lastSummary NOTIFY lastSummaryChanged)
    Q_PROPERTY(QString lastPayload READ lastPayload NOTIFY lastPayloadChanged)
    Q_PROPERTY(QString lastResponse READ lastResponse NOTIFY lastResponseChanged)
    Q_PROPERTY(QString suggestedPython READ suggestedPython NOTIFY suggestedPythonChanged)
    Q_PROPERTY(
        int recordedCommandCount READ recordedCommandCount NOTIFY recordedCommandCountChanged)
    Q_PROPERTY(QString recordedCommands READ recordedCommands NOTIFY recordedCommandsChanged)
    Q_PROPERTY(QString lastPythonOutput READ lastPythonOutput NOTIFY lastPythonOutputChanged)
    Q_PROPERTY(bool operationActive READ operationActive NOTIFY operationActiveChanged)
    Q_PROPERTY(double operationProgress READ operationProgress NOTIFY operationProgressChanged)
    Q_PROPERTY(QString operationMessage READ operationMessage NOTIFY operationMessageChanged)
    Q_PROPERTY(QString operationState READ operationState NOTIFY operationStateChanged)
    Q_PROPERTY(QAbstractItemModel* operationLogModel READ operationLogModel CONSTANT)
    Q_PROPERTY(QObject* operationLogService READ operationLogService CONSTANT)
    Q_PROPERTY(bool hasUnreadOperationErrors READ hasUnreadOperationErrors NOTIFY
                   hasUnreadOperationErrorsChanged)
    Q_PROPERTY(bool hasUnreadOperationLogs READ hasUnreadOperationLogs NOTIFY
                   hasUnreadOperationLogsChanged)

public:
    explicit OpenGeoLabController(QObject* parent = nullptr);
    ~OpenGeoLabController() override;

    [[nodiscard]] auto lastModule() const -> const QString&;
    [[nodiscard]] auto lastAction() const -> const QString&;
    [[nodiscard]] auto lastRequest() const -> const QString&;
    [[nodiscard]] auto lastStatus() const -> const QString&;
    [[nodiscard]] auto lastSummary() const -> const QString&;
    [[nodiscard]] auto lastPayload() const -> const QString&;
    [[nodiscard]] auto lastResponse() const -> const QString&;
    [[nodiscard]] auto suggestedPython() const -> const QString&;
    [[nodiscard]] auto recordedCommandCount() const -> int;
    [[nodiscard]] auto recordedCommands() const -> const QString&;
    [[nodiscard]] auto lastPythonOutput() const -> const QString&;
    [[nodiscard]] auto operationActive() const -> bool;
    [[nodiscard]] auto operationProgress() const -> double;
    [[nodiscard]] auto operationMessage() const -> const QString&;
    [[nodiscard]] auto operationState() const -> const QString&;
    [[nodiscard]] auto operationLogModel() const -> QAbstractItemModel*;
    [[nodiscard]] auto operationLogService() const -> QObject*;
    [[nodiscard]] auto hasUnreadOperationErrors() const -> bool;
    [[nodiscard]] auto hasUnreadOperationLogs() const -> bool;

    auto executeCommand(const nlohmann::json& request_json, const std::string& source)
        -> nlohmann::json;
    auto replayRecordedCommandsJson() -> nlohmann::json;
    auto clearRecordedCommandsJson() -> nlohmann::json;
    [[nodiscard]] auto applicationStateJson() const -> nlohmann::json;

    Q_INVOKABLE bool runServiceRequest(const QString& request_json);
    Q_INVOKABLE int submitServiceRequest(const QString& request_json);
    Q_INVOKABLE void markOperationLogSeen();
    Q_INVOKABLE void clearOperationLog();
    Q_INVOKABLE void replayRecordedCommands();
    Q_INVOKABLE void clearRecordedCommands();
    Q_INVOKABLE bool exportRecordedScript(const QString& file_path);
    Q_INVOKABLE void runEmbeddedPython(const QString& script);
    Q_INVOKABLE void runEmbeddedPythonCommandLine(const QString& command_line);
    Q_INVOKABLE void postUiNotice(int level,
                                  const QString& source,
                                  const QString& message,
                                  const QString& detail = QString());

signals:
    void lastModuleChanged();
    void lastActionChanged();
    void lastRequestChanged();
    void lastStatusChanged();
    void lastSummaryChanged();
    void lastPayloadChanged();
    void lastResponseChanged();
    void suggestedPythonChanged();
    void recordedCommandCountChanged();
    void recordedCommandsChanged();
    void lastPythonOutputChanged();
    void operationActiveChanged();
    void operationProgressChanged();
    void operationMessageChanged();
    void operationStateChanged();
    void serviceRequestFinished(int requestId, bool success);
    void hasUnreadOperationErrorsChanged();
    void hasUnreadOperationLogsChanged();

private:
    auto executeRequestEnvelope(const nlohmann::json& request_json,
                                const QString& source,
                                bool failIfAsyncActive) -> nlohmann::json;
    void applyStateDelta(const ControllerStateDelta& delta);
    void appendOperationLog(int level,
                            const QString& source,
                            const QString& message,
                            const QString& detail = QString(),
                            std::source_location location = std::source_location::current());
    void setOperationFeedback(bool active,
                              double progress,
                              const QString& message,
                              const QString& state);
    void resetOperationFeed();

    std::unique_ptr<OperationLogService> m_operationLogService;
    std::unique_ptr<OpenGeoLabRequestExecutor> m_requestExecutor;
    std::unique_ptr<OpenGeoLabFeedbackCoordinator> m_feedbackCoordinator;
    std::unique_ptr<OpenGeoLabAutomationFacade> m_automationFacade;
    QString m_lastModule;
    QString m_lastAction;
    QString m_lastRequest;
    QString m_lastStatus;
    QString m_lastSummary;
    QString m_lastPayload;
    QString m_lastResponse;
    QString m_suggestedPython;
    QString m_recordedCommands;
    int m_recordedCommandCount = 0;
    QString m_lastPythonOutput;
    bool m_operationActive = false;
    double m_operationProgress = 0.0;
    QString m_operationMessage;
    QString m_operationState = QStringLiteral("idle");
    bool m_suppressUiActivity = false;
};

} // namespace OGL::App
