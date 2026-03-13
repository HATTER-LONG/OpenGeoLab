/**
 * @file OpenGeoLabController.hpp
 * @brief QML-facing controller that drives the placeholder geometry pipeline.
 */

#pragma once

#include <QObject>
#include <QString>

#include <nlohmann/json.hpp>

#include <memory>

namespace ogl::python_wrapper {
class OpenGeoLabPythonBridge;
} // namespace ogl::python_wrapper

namespace ogl::app {

/**
 * @brief Thin application controller exposing the component pipeline to QML.
 */
class OpenGeoLabController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastModule READ lastModule NOTIFY lastModuleChanged)
    Q_PROPERTY(QString lastStatus READ lastStatus NOTIFY lastStatusChanged)
    Q_PROPERTY(QString lastSummary READ lastSummary NOTIFY lastSummaryChanged)
    Q_PROPERTY(QString lastPayload READ lastPayload NOTIFY lastPayloadChanged)
    Q_PROPERTY(QString suggestedPython READ suggestedPython NOTIFY suggestedPythonChanged)

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

    /**
     * @brief Execute a generic service request chosen by QML through module name and JSON payload.
     * @param module_name Target module name resolved through Kangaroo factories.
     * @param request_json JSON payload string passed from QML.
     */
    Q_INVOKABLE void runServiceRequest(const QString& module_name, const QString& request_json);

signals:
    void lastModuleChanged();
    void lastStatusChanged();
    void lastSummaryChanged();
    void lastPayloadChanged();
    void suggestedPythonChanged();

private:
    void updateFromResponse(const QString& module_name, const nlohmann::json& response);

    std::unique_ptr<ogl::python_wrapper::OpenGeoLabPythonBridge> m_pythonBridge;
    QString m_lastModule;
    QString m_lastStatus;
    QString m_lastSummary;
    QString m_lastPayload;
    QString m_suggestedPython;
};

} // namespace ogl::app