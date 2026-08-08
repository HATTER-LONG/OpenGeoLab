/**
 * @file qml_page_registration_test.cpp
 * @brief Verifies geometry creation pages are registered in app QML resources.
 */

#include <doctest/doctest.h>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqml.h>

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace OpenGeoLab::App::Tests {

namespace Fs = std::filesystem;

class FakeRequestService final : public QObject {
    Q_OBJECT

Q_SIGNALS:
    void responseReady(const QString& response_json, bool muted, quint64 request_id);
    void errorOccurred(const QString& error_message, bool muted, quint64 request_id);
    void requestSent(const QString& description,
                     const QString& request_json,
                     bool muted,
                     quint64 request_id);
    void progressUpdated(double progress, const QString& message, quint64 request_id);
};

static std::string readFile(const Fs::path& path) {
    const std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

static void requireSnippets(const std::string& content,
                            std::initializer_list<std::string_view> snippets) {
    for(const auto snippet : snippets) {
        INFO("Missing snippet: " << snippet);
        CHECK(content.find(snippet) != std::string::npos);
    }
}

static Fs::path appSourceDir() { return Fs::path{OPENGEOLAB_APP_SOURCE_DIR}; }

static QGuiApplication& ensureQmlApplication() {
    if(auto* application = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        return *application;
    }

    qputenv("QT_QPA_PLATFORM", "offscreen");
    static int argc = 1;
    static char app_name[] = "opengeolab_qml_progress_test";
    static char* argv[] = {app_name, nullptr};
    static auto application = std::make_unique<QGuiApplication>(argc, argv);
    return *application;
}

TEST_CASE("MainPages registers all geometry creation pages") {
    const auto main_pages_path = appSourceDir() / "resource/qml/MainPages.qml";
    REQUIRE_MESSAGE(Fs::exists(main_pages_path), "Missing file: " << main_pages_path.string());

    const auto content = readFile(main_pages_path);
    requireSnippets(content,
                    {"\"addBox\":      { path: \"components/pages/CreateBoxPage.qml\" }",
                     "\"addCylinder\": { path: \"components/pages/CreateCylinderPage.qml\" }",
                     "\"addSphere\":   { path: \"components/pages/CreateSpherePage.qml\" }",
                     "\"addTorus\":    { path: \"components/pages/CreateTorusPage.qml\" }"});
}

TEST_CASE("App CMake packages all geometry page QML files") {
    const auto cmake_path = appSourceDir() / "CMakeLists.txt";
    REQUIRE_MESSAGE(Fs::exists(cmake_path), "Missing file: " << cmake_path.string());

    const auto content = readFile(cmake_path);
    requireSnippets(content, {"resource/qml/components/pages/CreateBoxPage.qml",
                              "resource/qml/components/pages/CreateCylinderPage.qml",
                              "resource/qml/components/pages/CreateSpherePage.qml",
                              "resource/qml/components/pages/CreateTorusPage.qml"});
}

TEST_CASE("CreateCylinderPage defines the expected cylinder workflow") {
    const auto page_path = appSourceDir() / "resource/qml/components/pages/CreateCylinderPage.qml";
    REQUIRE_MESSAGE(Fs::exists(page_path), "Missing file: " << page_path.string());

    const auto content = readFile(page_path);
    requireSnippets(content, {"pageTitle: qsTr(\"Create Cylinder\")", "pageIcon: \"cylinder\"",
                              "actionId: \"addCylinder\"", "property string cylinderName: \"\"",
                              "property real centerX: 0.0", "property real centerY: 0.0",
                              "property real centerZ: 0.0", "property real radius: 5.0",
                              "property real cylHeight: 10.0", "label: qsTr(\"Cylinder Name\")",
                              "label: qsTr(\"Center Point\")", "tooltipText: qsTr(\"Radius\")",
                              "tooltipText: qsTr(\"Height\")", "text: qsTr(\"Volume:\")",
                              "text: qsTr(\"Surface Area:\")", "action: \"create_cylinder\"",
                              "radius: root.radius, height: root.cylHeight"});
}

TEST_CASE("CreateSpherePage defines the expected sphere workflow") {
    const auto page_path = appSourceDir() / "resource/qml/components/pages/CreateSpherePage.qml";
    REQUIRE_MESSAGE(Fs::exists(page_path), "Missing file: " << page_path.string());

    const auto content = readFile(page_path);
    requireSnippets(content, {"pageTitle: qsTr(\"Create Sphere\")", "pageIcon: \"sphere\"",
                              "actionId: \"addSphere\"", "property string sphereName: \"\"",
                              "property real centerX: 0.0", "property real centerY: 0.0",
                              "property real centerZ: 0.0", "property real radius: 5.0",
                              "label: qsTr(\"Sphere Name\")", "label: qsTr(\"Center Point\")",
                              "tooltipText: qsTr(\"Radius\")", "text: qsTr(\"Volume:\")",
                              "text: qsTr(\"Surface Area:\")", "text: qsTr(\"Diameter:\")",
                              "action: \"create_sphere\"", "radius: root.radius"});
}

TEST_CASE("CreateTorusPage defines the expected torus workflow") {
    const auto page_path = appSourceDir() / "resource/qml/components/pages/CreateTorusPage.qml";
    REQUIRE_MESSAGE(Fs::exists(page_path), "Missing file: " << page_path.string());

    const auto content = readFile(page_path);
    requireSnippets(content, {"pageTitle: qsTr(\"Create Torus\")",
                              "pageIcon: \"torus\"",
                              "actionId: \"addTorus\"",
                              "property string torusName: \"\"",
                              "property real centerX: 0.0",
                              "property real centerY: 0.0",
                              "property real centerZ: 0.0",
                              "property real majorRadius: 10.0",
                              "property real minorRadius: 3.0",
                              "label: qsTr(\"Torus Name\")",
                              "label: qsTr(\"Center Point\")",
                              "tooltipText: qsTr(\"Major Radius\")",
                              "tooltipText: qsTr(\"Minor Radius\")",
                              "visible: root.minorRadius >= root.majorRadius",
                              "text: qsTr(\"Minor radius should be less than major radius\")",
                              "text: qsTr(\"Volume:\")",
                              "text: qsTr(\"Surface Area:\")",
                              "text: qsTr(\"Outer Diameter:\")",
                              "action: \"create_torus\"",
                              "majorRadius: root.majorRadius, minorRadius: root.minorRadius"});
}

TEST_CASE("Chinese translations cover the new geometry pages") {
    const auto translation_path = appSourceDir() / "resource/translations/opengeolab_zh_CN.ts";
    REQUIRE_MESSAGE(Fs::exists(translation_path), "Missing file: " << translation_path.string());

    const auto content = readFile(translation_path);
    requireSnippets(content, {"<name>CreateCylinderPage</name>", "<source>Create Cylinder</source>",
                              "<source>Cylinder Name</source>", "<source>Center Point</source>",
                              "<source>Radius</source>", "<source>Height</source>",
                              "<source>Surface Area:</source>", "<name>CreateSpherePage</name>",
                              "<source>Create Sphere</source>", "<source>Sphere Name</source>",
                              "<source>Diameter:</source>", "<name>CreateTorusPage</name>",
                              "<source>Create Torus</source>", "<source>Torus Name</source>",
                              "<source>Major Radius</source>", "<source>Minor Radius</source>",
                              "<source>Outer Diameter:</source>",
                              "<source>Minor radius should be less than major radius</source>"});
}

TEST_CASE("ProgressCard supports determinate and indeterminate request states") {
    const auto progress_path = appSourceDir() / "resource/qml/components/ProgressCard.qml";
    REQUIRE_MESSAGE(Fs::exists(progress_path), "Missing file: " << progress_path.string());

    const auto content = readFile(progress_path);
    requireSnippets(
        content,
        {"property bool hasDeterminateProgress: false", "property double requestId: -1",
         "readonly property bool indeterminate:", "root.active && !root.hasDeterminateProgress",
         "if (root.completionState === \"done\")", "root.completionState !== \"failed\"",
         "requestId !== root.requestId", "hideTimer.stop()"});
}

TEST_CASE("Muted requests do not publish visible progress") {
    const auto service_path = appSourceDir() / "src/request_service.cpp";
    REQUIRE_MESSAGE(Fs::exists(service_path), "Missing file: " << service_path.string());

    const auto content = readFile(service_path);
    requireSnippets(content, {"[this, muted, request_id](double progress", "if(muted) {",
                              "Q_EMIT progressUpdated(progress, msg, request_id);"});
}

TEST_CASE("ProgressCard stops a previous completion timer for a new request") {
    (void)ensureQmlApplication();
    FakeRequestService service;
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "RequestService", &service);

    QQmlEngine engine;
    const auto qml_root = appSourceDir() / "resource/qml";

    QQmlComponent theme_component(&engine, QUrl::fromLocalFile(QString::fromStdString(
                                               (qml_root / "theme/AppTheme.qml").string())));
    INFO(theme_component.errorString().toStdString());
    std::unique_ptr<QObject> theme(theme_component.create());
    REQUIRE(theme != nullptr);

    QQmlComponent card_component(
        &engine, QUrl::fromLocalFile(
                     QString::fromStdString((qml_root / "components/ProgressCard.qml").string())));
    INFO(card_component.errorString().toStdString());
    std::unique_ptr<QObject> card(
        card_component.createWithInitialProperties({{"theme", QVariant::fromValue(theme.get())}}));
    REQUIRE(card != nullptr);

    Q_EMIT service.requestSent(QStringLiteral("first.action"), QStringLiteral("{}"), false, 1);
    Q_EMIT service.responseReady(QStringLiteral(R"({"ok":true})"), false, 1);
    QCoreApplication::processEvents();

    auto* hide_timer = card->findChild<QObject*>(QStringLiteral("hideTimer"));
    REQUIRE(hide_timer != nullptr);
    CHECK(hide_timer->property("running").toBool());

    Q_EMIT service.requestSent(QStringLiteral("second.action"), QStringLiteral("{}"), false, 2);
    QCoreApplication::processEvents();

    CHECK_FALSE(hide_timer->property("running").toBool());
    CHECK(card->property("active").toBool());
    CHECK(card->property("lastDescription").toString() == QStringLiteral("second.action"));
}

} // namespace OpenGeoLab::App::Tests

#include "qml_page_registration_test.moc"
