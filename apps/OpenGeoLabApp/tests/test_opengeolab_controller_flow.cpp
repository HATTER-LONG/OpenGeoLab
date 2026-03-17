#include <catch2/catch_test_macros.hpp>

#include <ogl/app/OpenGeoLabController.hpp>
#include <ogl/command/RequestProtocol.hpp>

#include "../src/OpenGeoLabAutomationFacade.hpp"
#include "../src/OpenGeoLabFeedbackCoordinator.hpp"
#include "../src/OpenGeoLabRequestExecutor.hpp"
#include "../src/OperationLogModel.hpp"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageLogContext>
#include <QModelIndex>
#include <QQmlComponent>
#include <QQmlEngine>

#include <future>
#include <memory>
#include <vector>

namespace {

class ScopedQtMessageCapture {
public:
    ScopedQtMessageCapture()
        : m_previousHandler(qInstallMessageHandler(&ScopedQtMessageCapture::handler)) {
        s_activeCapture = this;
    }

    ~ScopedQtMessageCapture() {
        s_activeCapture = nullptr;
        qInstallMessageHandler(m_previousHandler);
    }

    [[nodiscard]] auto containsWarningSubstring(const QString& needle) const -> bool {
        return std::any_of(m_messages.begin(), m_messages.end(),
                           [&needle](const QString& message) { return message.contains(needle); });
    }

private:
    static void handler(QtMsgType, const QMessageLogContext&, const QString& message) {
        if(s_activeCapture != nullptr) {
            s_activeCapture->m_messages.push_back(message);
        }
    }

    inline static ScopedQtMessageCapture* s_activeCapture = nullptr;
    QtMessageHandler m_previousHandler = nullptr;
    std::vector<QString> m_messages;
};

auto ensureGuiApplication() -> QGuiApplication& {
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));

    static int argc = 1;
    static char app_name[] = "opengeolab_controller_flow_test";
    static char* argv[] = {app_name, nullptr};
    static QGuiApplication app(argc, argv);
    return app;
}

auto qmlFilePath(const QString& relativePath) -> QString {
    const QDir testsDir = QFileInfo(QString::fromUtf8(__FILE__)).dir();
    const QDir qmlRootDir(testsDir.filePath(QStringLiteral("../qml")));
    return qmlRootDir.absoluteFilePath(relativePath);
}

auto componentErrors(const QQmlComponent& component) -> QString {
    QStringList messages;
    for(const QQmlError& error : component.errors()) {
        messages.push_back(error.toString());
    }
    return messages.join(QLatin1Char('\n'));
}

} // namespace

TEST_CASE("controller applies response state before serviceRequestFinished",
          "[app][controller][flow]") {
    static_cast<void>(ensureGuiApplication());

    OGL::App::OpenGeoLabController controller;
    bool finished_seen = false;

    QObject::connect(&controller, &OGL::App::OpenGeoLabController::serviceRequestFinished,
                     &controller, [&controller, &finished_seen](int requestId, bool success) {
                         finished_seen = true;
                         CHECK(requestId > 0);
                         CHECK(success);
                         CHECK_FALSE(controller.lastResponse().isEmpty());
                         CHECK(controller.operationState() == QStringLiteral("success"));
                     });

    const int request_id = controller.submitServiceRequest(QStringLiteral(R"JSON({
  "module": "geometry",
  "action": "createBox",
  "param": {
    "modelName": "ControllerFlowBox",
    "origin": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "dimensions": { "x": 10.0, "y": 12.0, "z": 14.0 }
  }
})JSON"));
    REQUIRE(request_id > 0);

    QElapsedTimer timer;
    timer.start();
    while(!finished_seen && timer.elapsed() < 5000) {
        QGuiApplication::processEvents();
    }

    REQUIRE(finished_seen);
}

TEST_CASE("request executor reports deterministic busy failures", "[app][controller][executor]") {
    static_cast<void>(ensureGuiApplication());

    auto started_promise = std::make_shared<std::promise<void>>();
    auto unblock_promise = std::make_shared<std::promise<void>>();
    auto started_future = started_promise->get_future();
    auto unblock_future = unblock_promise->get_future().share();

    OGL::App::OpenGeoLabRequestExecutor executor(
        std::make_unique<OGL::Command::CommandRecorder>(),
        [started_promise, unblock_future](OGL::Command::CommandRecorder& recorder,
                                          const OGL::Command::CommandRequest& request,
                                          const OGL::Core::ProgressCallback& progressCallback) {
            started_promise->set_value();
            unblock_future.wait();
            return recorder.execute(request, progressCallback).toJson();
        });

    QObject callback_context;
    OGL::App::OpenGeoLabRequestExecutor::EventSinks event_sinks;
    int finished_request_id = -1;
    bool finished_success = false;
    event_sinks.onRequestFinished =
        [&finished_request_id, &finished_success](const OGL::App::RequestFinishedEvent& event) {
            if(event.requestId > 0) {
                finished_request_id = event.requestId;
                finished_success = event.response.value("success", false);
            }
        };

    const int request_id =
        executor.submitAsync(&callback_context,
                             OGL::Command::parseCommandRequest(
                                 {{"module", "geometry"},
                                  {"action", "createBox"},
                                  {"param",
                                   {{"modelName", "BusyBox"},
                                    {"origin", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
                                    {"dimensions", {{"x", 10.0}, {"y", 10.0}, {"z", 10.0}}}}}}),
                             QStringLiteral("qml-ui"), event_sinks);
    REQUIRE(request_id > 0);
    started_future.wait();

    const auto busy_response = executor.executeSync(
        OGL::Command::parseCommandRequest({{"module", "scene"}, {"action", "buildScene"}}),
        QStringLiteral("embedded-python"), event_sinks,
        OGL::App::OpenGeoLabRequestExecutor::BusyPolicy::FailIfAsyncActive);

    CHECK_FALSE(busy_response.value("success", true));
    CHECK(busy_response.value("message", std::string{}) ==
          "Another async service request is active.");
    CHECK(busy_response["payload"].value("reason", std::string{}) == "request-busy");

    unblock_promise->set_value();

    QElapsedTimer timer;
    timer.start();
    while(timer.elapsed() < 5000 &&
          (finished_request_id != request_id || executor.isAsyncRequestActive())) {
        QGuiApplication::processEvents();
    }

    REQUIRE(finished_request_id == request_id);
    CHECK(finished_success);
    CHECK_FALSE(executor.isAsyncRequestActive());
}

TEST_CASE("request executor observes fast async completion", "[app][controller][executor]") {
    static_cast<void>(ensureGuiApplication());

    ScopedQtMessageCapture message_capture;
    OGL::App::OpenGeoLabRequestExecutor executor(
        std::make_unique<OGL::Command::CommandRecorder>(),
        [](OGL::Command::CommandRecorder&, const OGL::Command::CommandRequest& request,
           const OGL::Core::ProgressCallback&) {
            return nlohmann::json{{"success", true},
                                  {"module", request.module},
                                  {"action", request.action},
                                  {"message", "fast async complete"},
                                  {"payload", nlohmann::json::object()}};
        });

    QObject callback_context;
    int finished_request_id = -1;
    bool finished_success = false;
    bool recorder_state_seen = false;
    OGL::App::OpenGeoLabRequestExecutor::EventSinks event_sinks;
    event_sinks.onRequestFinished =
        [&finished_request_id, &finished_success](const OGL::App::RequestFinishedEvent& event) {
            finished_request_id = event.requestId;
            finished_success = event.response.value("success", false);
        };
    event_sinks.onRecorderStateChanged =
        [&recorder_state_seen](const OGL::App::RecorderStateEvent&) { recorder_state_seen = true; };

    const int request_id = executor.submitAsync(
        &callback_context,
        OGL::Command::parseCommandRequest({{"module", "scene"}, {"action", "buildScene"}}),
        QStringLiteral("qml-ui"), event_sinks);
    REQUIRE(request_id > 0);

    QElapsedTimer timer;
    timer.start();
    while(timer.elapsed() < 1000 && (finished_request_id != request_id || !recorder_state_seen ||
                                     executor.isAsyncRequestActive())) {
        QGuiApplication::processEvents();
    }

    CHECK(finished_request_id == request_id);
    CHECK(finished_success);
    CHECK(recorder_state_seen);
    CHECK_FALSE(executor.isAsyncRequestActive());
    CHECK_FALSE(message_capture.containsWarningSubstring(
        QStringLiteral("connecting after calling setFuture() is likely to produce race")));
}

TEST_CASE("controller postUiNotice reaches visible activity state", "[app][controller][notice]") {
    static_cast<void>(ensureGuiApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();

    controller.postUiNotice(4, QStringLiteral("Qml.Router"), QStringLiteral("Unknown action"),
                            QStringLiteral("Action metadata was not registered."));

    CHECK(controller.lastSummary() == QStringLiteral("Unknown action"));
    CHECK(controller.operationState() == QStringLiteral("error"));
    REQUIRE(controller.operationLogModel() != nullptr);
    REQUIRE(controller.operationLogModel()->rowCount() >= 1);
    const QModelIndex latest_index =
        controller.operationLogModel()->index(controller.operationLogModel()->rowCount() - 1, 0);
    CHECK(latest_index.data(OGL::App::OperationLogModel::SourceRole).toString() ==
          QStringLiteral("Qml.Router"));
}

TEST_CASE("workflow router reports unknown action keys through controller notices",
          "[app][controller][notice][qml]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import ".." as AppRoot
import "../components" as Components

Item {
    id: root

    required property var appController
    visible: false
    property bool lastOpenResult: false
    property bool genericPageOpen: genericHost.open
    property bool geometryPageOpen: geometryHost.open

    AppRoot.ActionRegistry {
        id: actionRegistry
    }

    QtObject {
        id: genericHost
        property bool open: false
        property var actionDefinition: null

        function presentAction(actionDefinition) {
            genericHost.actionDefinition = actionDefinition;
            genericHost.open = true;
        }

        function refreshAction(actionDefinition) {
            genericHost.actionDefinition = actionDefinition;
        }
    }

    QtObject {
        id: geometryHost
        property bool open: false
        property var actionDefinition: null

        function presentAction(actionDefinition) {
            geometryHost.actionDefinition = actionDefinition;
            geometryHost.open = true;
        }

        function refreshAction(actionDefinition) {
            geometryHost.actionDefinition = actionDefinition;
        }
    }

    Components.ActionWorkflowRouter {
        id: router
        appController: root.appController
        actionRegistry: actionRegistry
        actionFeaturePage: genericHost
        geometryCreateFeaturePage: geometryHost
    }

    function triggerUnknownAction() {
        root.lastOpenResult = router.openAction("missingActionKey");
    }
}
)QML",
                      QUrl::fromLocalFile(qmlFilePath(QStringLiteral("tests/router_unknown_action_harness.qml"))));
    INFO(componentErrors(component).toStdString());
    REQUIRE(component.status() != QQmlComponent::Error);

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("appController"), QVariant::fromValue(&controller));

    auto harness = std::unique_ptr<QObject>(component.createWithInitialProperties(initialProperties));
    INFO(componentErrors(component).toStdString());
    REQUIRE(harness != nullptr);

    REQUIRE(QMetaObject::invokeMethod(harness.get(), "triggerUnknownAction"));
    CHECK_FALSE(harness->property("lastOpenResult").toBool());
    CHECK(controller.lastSummary() == QStringLiteral("Unknown action"));
    CHECK(controller.operationState() == QStringLiteral("error"));
    REQUIRE(controller.operationLogModel() != nullptr);
    REQUIRE(controller.operationLogModel()->rowCount() >= 1);
    const QModelIndex latest_index =
        controller.operationLogModel()->index(controller.operationLogModel()->rowCount() - 1, 0);
    CHECK(latest_index.data(OGL::App::OperationLogModel::SourceRole).toString() ==
          QStringLiteral("Qml.ActionWorkflowRouter"));
    CHECK(latest_index.data(OGL::App::OperationLogModel::MessageRole)
              .toString()
              .contains(QStringLiteral("missingActionKey")));

    CHECK_FALSE(harness->property("genericPageOpen").toBool());
    CHECK_FALSE(harness->property("geometryPageOpen").toBool());
}

TEST_CASE("ribbon model adapter skips unknown action keys with a controller warning",
          "[app][controller][notice][qml]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import ".." as AppRoot
import "../components" as Components

Item {
    id: root

    required property var appController
    visible: false

    AppRoot.ActionRegistry {
        id: actionRegistry
    }

    QtObject {
        id: ribbonConfig

        property var tabs: ["Geometry"]
        property var groupsModel: [[
            {
                "title": "Create",
                "actionKeys": ["addBox", "missingActionKey"]
            }
        ]]
    }

    Components.RibbonModelAdapter {
        id: adapter
        ribbonConfig: ribbonConfig
        actionRegistry: actionRegistry
        appController: root.appController
        reloadToken: "en_US"
    }

    readonly property var resolvedGroups: adapter.groupsForTab(0)
    readonly property int resolvedActionCount: resolvedGroups.length > 0 ? resolvedGroups[0].actions.length : 0
    readonly property string resolvedFirstActionKey: resolvedActionCount > 0 ? resolvedGroups[0].actions[0].key : ""
}
)QML",
                      QUrl::fromLocalFile(qmlFilePath(QStringLiteral("tests/ribbon_model_adapter_harness.qml"))));
    INFO(componentErrors(component).toStdString());
    REQUIRE(component.status() != QQmlComponent::Error);

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("appController"), QVariant::fromValue(&controller));

    auto harness = std::unique_ptr<QObject>(component.createWithInitialProperties(initialProperties));
    INFO(componentErrors(component).toStdString());
    REQUIRE(harness != nullptr);

    QGuiApplication::processEvents();

    CHECK(controller.lastSummary() == QStringLiteral("Ribbon action skipped"));
    CHECK(controller.operationState() == QStringLiteral("success"));
    REQUIRE(controller.operationLogModel() != nullptr);
    REQUIRE(controller.operationLogModel()->rowCount() >= 1);
    const QModelIndex latest_index =
        controller.operationLogModel()->index(controller.operationLogModel()->rowCount() - 1, 0);
    CHECK(latest_index.data(OGL::App::OperationLogModel::SourceRole).toString() ==
          QStringLiteral("Qml.RibbonModelAdapter"));
    CHECK(latest_index.data(OGL::App::OperationLogModel::MessageRole)
              .toString()
              .contains(QStringLiteral("missingActionKey")));
    CHECK(harness->property("resolvedActionCount").toInt() == 1);
    CHECK(harness->property("resolvedFirstActionKey").toString() == QStringLiteral("addBox"));
}

TEST_CASE("workflow router reports malformed action definitions through controller notices",
          "[app][controller][notice][qml]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import "../components" as Components

Item {
    id: root

    required property var appController
    visible: false
    property bool lastOpenResult: false
    property bool genericPageOpen: genericHost.open
    property bool geometryPageOpen: geometryHost.open

    QtObject {
        id: invalidRegistry

        function action(actionKey) {
            if (actionKey !== "invalidGeometry") {
                return null;
            }

            return {
                "key": "invalidGeometry",
                "pageTitle": "Invalid Geometry",
                "sectionTitle": "Geometry / Create",
                "icon": "box",
                "accent": "accentA",
                "summary": "Broken geometry request spec.",
                "nextMilestone": "Wire a valid request spec.",
                "focusPoints": ["Validation"],
                "workflowKind": "geometryCreate",
                "requestSpec": null
            };
        }
    }

    QtObject {
        id: genericHost
        property bool open: false
        property var actionDefinition: null

        function presentAction(actionDefinition) {
            genericHost.actionDefinition = actionDefinition;
            genericHost.open = true;
        }

        function refreshAction(actionDefinition) {
            genericHost.actionDefinition = actionDefinition;
        }
    }

    QtObject {
        id: geometryHost
        property bool open: false
        property var actionDefinition: null

        function presentAction(actionDefinition) {
            geometryHost.actionDefinition = actionDefinition;
            geometryHost.open = true;
        }

        function refreshAction(actionDefinition) {
            geometryHost.actionDefinition = actionDefinition;
        }
    }

    Components.ActionWorkflowRouter {
        id: router
        appController: root.appController
        actionRegistry: invalidRegistry
        actionFeaturePage: genericHost
        geometryCreateFeaturePage: geometryHost
    }

    function triggerInvalidAction() {
        root.lastOpenResult = router.openAction("invalidGeometry");
    }
}
)QML",
                      QUrl::fromLocalFile(qmlFilePath(QStringLiteral("tests/router_invalid_action_harness.qml"))));
    INFO(componentErrors(component).toStdString());
    REQUIRE(component.status() != QQmlComponent::Error);

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("appController"), QVariant::fromValue(&controller));

    auto harness = std::unique_ptr<QObject>(component.createWithInitialProperties(initialProperties));
    INFO(componentErrors(component).toStdString());
    REQUIRE(harness != nullptr);

    REQUIRE(QMetaObject::invokeMethod(harness.get(), "triggerInvalidAction"));
    CHECK_FALSE(harness->property("lastOpenResult").toBool());
    CHECK(controller.lastSummary() == QStringLiteral("Action configuration warning"));
    CHECK(controller.operationState() == QStringLiteral("success"));
    REQUIRE(controller.operationLogModel() != nullptr);
    REQUIRE(controller.operationLogModel()->rowCount() >= 1);
    const QModelIndex latest_index =
        controller.operationLogModel()->index(controller.operationLogModel()->rowCount() - 1, 0);
    CHECK(latest_index.data(OGL::App::OperationLogModel::SourceRole).toString() ==
          QStringLiteral("Qml.ActionWorkflowRouter"));
    CHECK(latest_index.data(OGL::App::OperationLogModel::LevelRole).toInt() == 3);
    CHECK(latest_index.data(OGL::App::OperationLogModel::MessageRole)
              .toString()
              .contains(QStringLiteral("invalidGeometry")));

    CHECK_FALSE(harness->property("genericPageOpen").toBool());
    CHECK_FALSE(harness->property("geometryPageOpen").toBool());
}

TEST_CASE("workflow router reports invalid field paths through controller notices",
          "[app][controller][notice][qml]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import ".." as AppRoot
import "../components" as Components

Item {
    id: root

    required property var appController
    visible: false

    AppRoot.ActionRegistry {
        id: actionRegistry
    }

    QtObject {
        id: invalidRegistry

        readonly property var invalidAction: actionRegistry.buildCatalogState([
            actionRegistry.createActionDefinition(
                "invalidFieldPath",
                "Broken",
                "Broken Path",
                "Geometry / Create",
                "box",
                "accentA",
                "Broken field path definition.",
                "Fix the path.",
                ["Validation"],
                "geometryCreate",
                {
                    "module": "geometry",
                    "action": "createBox",
                    "shapeType": "box",
                    "defaultName": "Broken_001",
                    "positionTitle": "Origin",
                    "positionFields": [
                        {
                            "key": "originX",
                            "label": "X",
                            "defaultValue": "0.0",
                            "unit": "mm",
                            "positiveOnly": false,
                            "accent": "accentD",
                            "path": ["origin.x"]
                        }
                    ],
                    "dimensionTitle": "Dimensions",
                    "dimensionFields": [
                        {
                            "key": "sizeX",
                            "label": "X",
                            "defaultValue": "10.0",
                            "unit": "mm",
                            "positiveOnly": true,
                            "accent": "accentD",
                            "path": ["dimensions", "x"]
                        }
                    ],
                    "axisOptions": [],
                    "defaultAxis": "Z"
                }
            )
        ]).lookup.invalidFieldPath

        function action(actionKey) {
            if (actionKey === "invalidFieldPath") {
                return invalidAction;
            }
            return null;
        }
    }

    QtObject {
        id: genericHost
        property bool open: false
        property var actionDefinition: null

        function presentAction(actionDefinition) {
            genericHost.actionDefinition = actionDefinition;
            genericHost.open = true;
        }

        function refreshAction(actionDefinition) {
            genericHost.actionDefinition = actionDefinition;
        }
    }

    QtObject {
        id: geometryHost
        property bool open: false
        property var actionDefinition: null

        function presentAction(actionDefinition) {
            geometryHost.actionDefinition = actionDefinition;
            geometryHost.open = true;
        }

        function refreshAction(actionDefinition) {
            geometryHost.actionDefinition = actionDefinition;
        }
    }

    Components.ActionWorkflowRouter {
        id: router
        appController: root.appController
        actionRegistry: invalidRegistry
        actionFeaturePage: genericHost
        geometryCreateFeaturePage: geometryHost
    }

    property bool genericPageOpen: genericHost.open
    property bool geometryPageOpen: geometryHost.open
    property string invalidConfigError: invalidRegistry.invalidAction ? invalidRegistry.invalidAction.configError : ""

    function triggerInvalidAction() {
        return router.openAction("invalidFieldPath");
    }
}
)QML",
                      QUrl::fromLocalFile(qmlFilePath(QStringLiteral("tests/router_invalid_field_path_harness.qml"))));
    INFO(componentErrors(component).toStdString());
    REQUIRE(component.status() != QQmlComponent::Error);

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("appController"), QVariant::fromValue(&controller));

    auto harness = std::unique_ptr<QObject>(component.createWithInitialProperties(initialProperties));
    INFO(componentErrors(component).toStdString());
    REQUIRE(harness != nullptr);

    const QString config_error = harness->property("invalidConfigError").toString();
    CHECK_FALSE(config_error.isEmpty());
    CHECK(config_error.contains(QStringLiteral("path")));

    QVariant opened;
    REQUIRE(QMetaObject::invokeMethod(harness.get(), "triggerInvalidAction",
                                      Q_RETURN_ARG(QVariant, opened)));
    CHECK_FALSE(opened.toBool());
    CHECK(controller.lastSummary() == QStringLiteral("Action configuration warning"));
    CHECK(controller.operationState() == QStringLiteral("success"));
    REQUIRE(controller.operationLogModel() != nullptr);
    REQUIRE(controller.operationLogModel()->rowCount() >= 1);
    const QModelIndex latest_index =
        controller.operationLogModel()->index(controller.operationLogModel()->rowCount() - 1, 0);
    CHECK(latest_index.data(OGL::App::OperationLogModel::SourceRole).toString() ==
          QStringLiteral("Qml.ActionWorkflowRouter"));
    CHECK(latest_index.data(OGL::App::OperationLogModel::MessageRole)
              .toString()
              .contains(QStringLiteral("path")));

    CHECK_FALSE(harness->property("genericPageOpen").toBool());
    CHECK_FALSE(harness->property("geometryPageOpen").toBool());
}

TEST_CASE("embedded python command line keeps request response ahead of trailing output",
          "[app][controller][python]") {
    static_cast<void>(ensureGuiApplication());

    OGL::App::OpenGeoLabController controller;

    controller.runEmbeddedPythonCommandLine(QStringLiteral(
        "import opengeolab_app\n"
        "result = opengeolab_app.process({'module': 'scene', 'action': 'buildScene'})\n"
        "print('controller-flow-tail')"));

    CHECK(controller.lastModule() == QStringLiteral("scene"));
    CHECK(controller.lastAction() == QStringLiteral("buildScene"));
    CHECK(controller.lastResponse().contains(QStringLiteral("\"success\": true")));
    CHECK(controller.lastPythonOutput().contains(QStringLiteral("controller-flow-tail")));
}
