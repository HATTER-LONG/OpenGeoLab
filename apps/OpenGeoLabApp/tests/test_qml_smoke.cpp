#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMetaObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariantMap>

#include <memory>

namespace {

auto ensureGuiApplication() -> QGuiApplication& {
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));

    static int argc = 1;
    static char app_name[] = "opengeolab_qml_smoke_test";
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

TEST_CASE("ViewportPanel instantiates with the current ribbon accent contract",
          "[app][qml][smoke]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;

    QQmlComponent themeComponent(&engine,
                                 QUrl::fromLocalFile(qmlFilePath(QStringLiteral("theme/AppTheme.qml"))));
    INFO(componentErrors(themeComponent).toStdString());
    REQUIRE(themeComponent.isReady());

    auto themeObject = std::unique_ptr<QObject>(themeComponent.create());
    REQUIRE(themeObject != nullptr);

    QQmlComponent viewportPanelComponent(
        &engine, QUrl::fromLocalFile(qmlFilePath(QStringLiteral("sections/ViewportPanel.qml"))));
    INFO(componentErrors(viewportPanelComponent).toStdString());
    REQUIRE(viewportPanelComponent.status() != QQmlComponent::Error);

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("theme"), QVariant::fromValue(themeObject.get()));
    initialProperties.insert(QStringLiteral("summaryText"), QStringLiteral("Viewport ready"));
    initialProperties.insert(QStringLiteral("recordedCommandCount"), 2);

    auto viewportPanelObject = std::unique_ptr<QObject>(
        viewportPanelComponent.createWithInitialProperties(initialProperties));
    INFO(componentErrors(viewportPanelComponent).toStdString());
    REQUIRE(viewportPanelObject != nullptr);
}

TEST_CASE("GeometryCreatePageState preserves form input across same-action metadata refresh",
          "[app][qml][smoke]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import ".." as AppRoot
import "../components" as Components

Item {
    id: root

    visible: false
    property var currentAction: actionRegistry.action("addCylinder")

    AppRoot.ActionRegistry {
        id: actionRegistry
    }

    Components.GeometryCreatePageState {
        id: pageState
        actionDefinition: root.currentAction
    }

    function seedAndRefresh() {
        pageState.setFieldValue("modelName", "Cylinder_Keep");
        pageState.setFieldValue("radius", "41.5");
        pageState.axisValue = "Y";
        root.currentAction = actionRegistry.action("addCylinder");
    }

    readonly property string modelNameValue: pageState.fieldValue("modelName")
    readonly property string radiusValue: pageState.fieldValue("radius")
    readonly property string axisValue: pageState.axisValue
    readonly property string requestPreview: pageState.requestJson
}
)QML",
                      QUrl::fromLocalFile(
                          qmlFilePath(QStringLiteral("tests/geometry_create_page_state_harness.qml"))));
    INFO(componentErrors(component).toStdString());
    REQUIRE(component.status() != QQmlComponent::Error);

    auto harness = std::unique_ptr<QObject>(component.create());
    INFO(componentErrors(component).toStdString());
    REQUIRE(harness != nullptr);

    REQUIRE(QMetaObject::invokeMethod(harness.get(), "seedAndRefresh"));
    CHECK(harness->property("modelNameValue").toString() == QStringLiteral("Cylinder_Keep"));
    CHECK(harness->property("radiusValue").toString() == QStringLiteral("41.5"));
    CHECK(harness->property("axisValue").toString() == QStringLiteral("Y"));
    CHECK(harness->property("requestPreview").toString().contains(QStringLiteral("Cylinder_Keep")));
    CHECK(harness->property("requestPreview").toString().contains(QStringLiteral("\"axis\":\"Y\"")));
}

TEST_CASE("GeometryCreateFeaturePage preserves form input across same-action refresh",
          "[app][qml][smoke]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;

    QQmlComponent themeComponent(&engine,
                                 QUrl::fromLocalFile(qmlFilePath(QStringLiteral("theme/AppTheme.qml"))));
    INFO(componentErrors(themeComponent).toStdString());
    REQUIRE(themeComponent.isReady());
    auto themeObject = std::unique_ptr<QObject>(themeComponent.create());
    REQUIRE(themeObject != nullptr);

    QQmlComponent controllerComponent(&engine);
    controllerComponent.setData(R"QML(
import QtQml

QtObject {
    property string lastSummary: ""
    signal serviceRequestFinished(int requestId, bool success)

    function submitServiceRequest(requestJson) {
        return 1;
    }
}
)QML",
                                QUrl::fromLocalFile(qmlFilePath(QStringLiteral("tests/fake_geometry_controller.qml"))));
    INFO(componentErrors(controllerComponent).toStdString());
    REQUIRE(controllerComponent.status() != QQmlComponent::Error);
    auto controllerObject = std::unique_ptr<QObject>(controllerComponent.create());
    REQUIRE(controllerObject != nullptr);

    QQmlComponent registryComponent(&engine,
                                    QUrl::fromLocalFile(qmlFilePath(QStringLiteral("ActionRegistry.qml"))));
    INFO(componentErrors(registryComponent).toStdString());
    REQUIRE(registryComponent.isReady());
    auto registryObject = std::unique_ptr<QObject>(registryComponent.create());
    REQUIRE(registryObject != nullptr);

    QQmlComponent pageComponent(
        &engine,
        QUrl::fromLocalFile(qmlFilePath(QStringLiteral("components/GeometryCreateFeaturePage.qml"))));
    INFO(componentErrors(pageComponent).toStdString());
    REQUIRE(pageComponent.status() != QQmlComponent::Error);

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("theme"), QVariant::fromValue(themeObject.get()));
    initialProperties.insert(QStringLiteral("appController"), QVariant::fromValue(controllerObject.get()));

    auto pageObject = std::unique_ptr<QObject>(pageComponent.createWithInitialProperties(initialProperties));
    INFO(componentErrors(pageComponent).toStdString());
    REQUIRE(pageObject != nullptr);

    QVariant actionDefinition;
    REQUIRE(QMetaObject::invokeMethod(registryObject.get(), "action",
                                      Q_RETURN_ARG(QVariant, actionDefinition),
                                      Q_ARG(QVariant, QStringLiteral("addCylinder"))));
    REQUIRE(actionDefinition.isValid());
    REQUIRE(QMetaObject::invokeMethod(pageObject.get(), "presentAction",
                                      Q_ARG(QVariant, actionDefinition)));

    QObject* pageStateObject = pageObject->findChild<QObject*>(QStringLiteral("geometryCreatePageState"));
    REQUIRE(pageStateObject != nullptr);

    REQUIRE(QMetaObject::invokeMethod(pageStateObject, "setFieldValue",
                                      Q_ARG(QVariant, QStringLiteral("modelName")),
                                      Q_ARG(QVariant, QStringLiteral("Cylinder_Keep"))));
    REQUIRE(QMetaObject::invokeMethod(pageStateObject, "setFieldValue",
                                      Q_ARG(QVariant, QStringLiteral("radius")),
                                      Q_ARG(QVariant, QStringLiteral("41.5"))));
    pageStateObject->setProperty("axisValue", QStringLiteral("Y"));

    QVariant refreshedAction = actionDefinition;
    QVariantMap refreshedActionMap = refreshedAction.toMap();
    refreshedActionMap.insert(QStringLiteral("pageTitle"), QStringLiteral("Create Cylinder (Refreshed)"));
    refreshedAction = refreshedActionMap;

    REQUIRE(QMetaObject::invokeMethod(pageObject.get(), "refreshAction",
                                      Q_ARG(QVariant, refreshedAction)));

    QVariant modelNameValue;
    QVariant radiusValue;
    REQUIRE(QMetaObject::invokeMethod(pageStateObject, "fieldValue",
                                      Q_RETURN_ARG(QVariant, modelNameValue),
                                      Q_ARG(QVariant, QStringLiteral("modelName"))));
    REQUIRE(QMetaObject::invokeMethod(pageStateObject, "fieldValue",
                                      Q_RETURN_ARG(QVariant, radiusValue),
                                      Q_ARG(QVariant, QStringLiteral("radius"))));

    CHECK(modelNameValue.toString() == QStringLiteral("Cylinder_Keep"));
    CHECK(radiusValue.toString() == QStringLiteral("41.5"));
    CHECK(pageStateObject->property("axisValue").toString() == QStringLiteral("Y"));
    CHECK(pageStateObject->property("requestJson").toString().contains(QStringLiteral("Cylinder_Keep")));
    CHECK(pageObject->property("pageTitle").toString() ==
          QStringLiteral("Create Cylinder (Refreshed)"));
}

TEST_CASE("OperationCommandLineTab assembles request and response transcript entries",
          "[app][qml][smoke]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;

    QQmlComponent themeComponent(&engine,
                                 QUrl::fromLocalFile(qmlFilePath(QStringLiteral("theme/AppTheme.qml"))));
    INFO(componentErrors(themeComponent).toStdString());
    REQUIRE(themeComponent.isReady());
    auto themeObject = std::unique_ptr<QObject>(themeComponent.create());
    REQUIRE(themeObject != nullptr);

    QQmlComponent harnessComponent(&engine);
    harnessComponent.setData(R"QML(
import QtQuick
import "../components" as Components

Item {
    id: root

    required property var theme

    property string lastRequest: ""
    property string lastResponse: ""
    property string lastPythonOutput: ""
    property int nextRequestId: 1

    signal serviceRequestFinished(int requestId, bool success)

    function submitServiceRequest(requestJson) {
        lastRequest = requestJson;
        return nextRequestId++;
    }

    function runEmbeddedPythonCommandLine(commandText) {
        lastPythonOutput = "python-output:" + commandText;
    }

    function runJsonCommand() {
        commandTab.commandText = "{\"module\":\"geometry\",\"action\":\"inspectModel\",\"param\":{\"name\":\"Box\"}}";
        commandTab.runCommandLine();
    }

    function finishJsonCommand() {
        lastResponse = "{\"success\":true,\"payload\":{\"summary\":\"box-ready\"}}";
        serviceRequestFinished(1, true);
    }

    function runPythonCommand() {
        commandTab.commandText = "print('hello from python')";
        commandTab.runCommandLine();
    }

    readonly property int transcriptCount: commandTab.transcriptCount
    readonly property string transcript0: commandTab.transcriptEntryBody(0)
    readonly property string transcript1: commandTab.transcriptEntryBody(1)
    readonly property string transcript2: commandTab.transcriptEntryBody(2)
    readonly property string transcript3: commandTab.transcriptEntryBody(3)

    Components.OperationCommandLineTab {
        id: commandTab
        objectName: "operationCommandLineTab"
        theme: root.theme
        appController: root
    }
}
)QML",
                             QUrl::fromLocalFile(
                                 qmlFilePath(QStringLiteral("tests/operation_command_line_tab_harness.qml"))));
    INFO(componentErrors(harnessComponent).toStdString());
    REQUIRE(harnessComponent.status() != QQmlComponent::Error);

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("theme"), QVariant::fromValue(themeObject.get()));
    auto harness = std::unique_ptr<QObject>(
        harnessComponent.createWithInitialProperties(initialProperties));
    INFO(componentErrors(harnessComponent).toStdString());
    REQUIRE(harness != nullptr);

    REQUIRE(QMetaObject::invokeMethod(harness.get(), "runJsonCommand"));
    CHECK(harness->property("transcriptCount").toInt() == 1);
    CHECK(harness->property("transcript0").toString().contains(
        QStringLiteral("opengeolab_app.process")));

    REQUIRE(QMetaObject::invokeMethod(harness.get(), "finishJsonCommand"));
    CHECK(harness->property("transcriptCount").toInt() == 2);
    CHECK(harness->property("transcript1").toString().contains(QStringLiteral("\"summary\": \"box-ready\"")));

    REQUIRE(QMetaObject::invokeMethod(harness.get(), "runPythonCommand"));
    CHECK(harness->property("transcriptCount").toInt() == 4);
    CHECK(harness->property("transcript2").toString().contains(
        QStringLiteral("print('hello from python')")));
    CHECK(harness->property("transcript3").toString().contains(
        QStringLiteral("python-output:print('hello from python')")));
}

TEST_CASE("OperationLogPanel instantiates as a shell over split activity subcomponents",
          "[app][qml][smoke]") {
    static_cast<void>(ensureGuiApplication());

    QQmlEngine engine;

    QQmlComponent themeComponent(&engine,
                                 QUrl::fromLocalFile(qmlFilePath(QStringLiteral("theme/AppTheme.qml"))));
    INFO(componentErrors(themeComponent).toStdString());
    REQUIRE(themeComponent.isReady());
    auto themeObject = std::unique_ptr<QObject>(themeComponent.create());
    REQUIRE(themeObject != nullptr);

    QQmlComponent controllerComponent(&engine);
    controllerComponent.setData(R"QML(
import QtQuick

Item {
    id: root

    property bool hasUnreadOperationErrors: false
    property bool hasUnreadOperationLogs: true
    property string lastRequest: ""
    property string lastResponse: ""
    property string lastPythonOutput: ""
    property var operationLogService: fakeLogService

    signal serviceRequestFinished(int requestId, bool success)

    function submitServiceRequest(requestJson) {
        lastRequest = requestJson;
        return 1;
    }

    function runEmbeddedPythonCommandLine(commandText) {
        lastPythonOutput = commandText;
    }

    QtObject {
        id: fakeLogService

        property int enabledLevelMask: 0x3F
        property int minLevel: 2
        property var model: ListModel {
            id: logModel

            ListElement {
                level: 2
                levelName: "Info"
                source: "Geometry"
                message: "Created box"
                time: "12:00:00"
                threadId: 7
                file: "BuildSceneAction.cpp"
                line: 42
                functionName: "execute"
            }
        }

        function setMinLevel(level) {
            minLevel = level;
        }

        function setLevelEnabled(level, enabled) {
            const bit = 1 << level;
            enabledLevelMask = enabled ? (enabledLevelMask | bit) : (enabledLevelMask & ~bit);
        }

        function clear() {
            logModel.clear();
        }

        function markAllSeen() {}
    }
}
)QML",
                                QUrl::fromLocalFile(qmlFilePath(QStringLiteral("tests/fake_operation_log_controller.qml"))));
    INFO(componentErrors(controllerComponent).toStdString());
    REQUIRE(controllerComponent.status() != QQmlComponent::Error);
    auto controllerObject = std::unique_ptr<QObject>(controllerComponent.create());
    REQUIRE(controllerObject != nullptr);

    QQmlComponent panelComponent(
        &engine, QUrl::fromLocalFile(qmlFilePath(QStringLiteral("components/OperationLogPanel.qml"))));
    INFO(componentErrors(panelComponent).toStdString());
    REQUIRE(panelComponent.status() != QQmlComponent::Error);

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("theme"), QVariant::fromValue(themeObject.get()));
    initialProperties.insert(QStringLiteral("appController"), QVariant::fromValue(controllerObject.get()));
    initialProperties.insert(QStringLiteral("open"), true);
    initialProperties.insert(QStringLiteral("currentTab"), 1);

    auto panelObject = std::unique_ptr<QObject>(
        panelComponent.createWithInitialProperties(initialProperties));
    INFO(componentErrors(panelComponent).toStdString());
    REQUIRE(panelObject != nullptr);
}
