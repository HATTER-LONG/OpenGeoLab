#include <ogl/app/OpenGeoLabController.hpp>

#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>

#include <exception>
#include <nlohmann/json.hpp>

namespace ogl::app {

OpenGeoLabController::OpenGeoLabController(QObject* parent)
    : QObject(parent),
      m_pythonBridge(std::make_unique<ogl::python_wrapper::OpenGeoLabPythonBridge>()) {
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

void OpenGeoLabController::runServiceRequest(const QString& module_name,
                                             const QString& request_json) {
    const QString trimmed_module = module_name.trimmed();
    if(trimmed_module.isEmpty()) {
        updateFromResponse(QString(), nlohmann::json{{"success", false},
                                                     {"message", "Module name cannot be empty."},
                                                     {"payload", nlohmann::json::object()}});
        return;
    }

    try {
        nlohmann::json request = nlohmann::json::object();
        if(!request_json.trimmed().isEmpty()) {
            request = nlohmann::json::parse(request_json.toStdString());
        }

        if(!request.contains("source")) {
            request["source"] = "qml-ui";
        }

        updateFromResponse(trimmed_module,
                           m_pythonBridge->call(trimmed_module.toStdString(), request));
    } catch(const std::exception& ex) {
        updateFromResponse(trimmed_module, nlohmann::json{{"success", false},
                                                          {"message", ex.what()},
                                                          {"payload", nlohmann::json::object()}});
    }
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
    m_suggestedPython = QString::fromStdString(payload.value("equivalentPython", std::string{}));

    emit lastModuleChanged();
    emit lastStatusChanged();
    emit lastSummaryChanged();
    emit lastPayloadChanged();
    emit suggestedPythonChanged();
}

} // namespace ogl::app