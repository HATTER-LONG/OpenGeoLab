#include <catch2/catch_test_macros.hpp>

#include <ogl/app/OpenGeoLabController.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <string>

namespace {

auto ensureCoreApplication() -> QCoreApplication& {
    static int argc = 1;
    static char app_name[] = "opengeolab_embedded_python_smoke_test";
    static char* argv[] = {app_name, nullptr};
    static QCoreApplication app(argc, argv);
    return app;
}

} // namespace

TEST_CASE("embedded python runtime drives the command recorder", "[app][python][smoke]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearRecordedCommands();

    controller.runEmbeddedPython(
        QStringLiteral("import opengeolab_app\n\n"
                       "result = opengeolab_app.call(\n"
                       "    \"selection\",\n"
                       "    {\n"
                       "        \"operation\": \"pickPlaceholderEntity\",\n"
                       "        \"modelName\": \"EmbeddedPythonTest\",\n"
                       "        \"bodyCount\": 2,\n"
                       "        \"viewportWidth\": 800,\n"
                       "        \"viewportHeight\": 600,\n"
                       "        \"screenX\": 160,\n"
                       "        \"screenY\": 120\n"
                       "    },\n"
                       ")\n"
                       "print(result[\"payload\"][\"summary\"])\n"));

    REQUIRE(controller.recordedCommandCount() == 1);

    const auto state = controller.applicationStateJson();
    CHECK(state.value("lastModule", std::string{}) == "selection");
    CHECK(state.value("recordedCommandCount", 0) == 1);
    CHECK(state.value("lastSummary", std::string{}).find("Placeholder") != std::string::npos);
}

TEST_CASE("embedded python command line evaluates expressions and statements",
          "[app][python][cli]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearRecordedCommands();

    controller.recordSelectionSmokeTest();
    REQUIRE(controller.recordedCommandCount() == 1);

    controller.runEmbeddedPythonCommandLine(
        QStringLiteral("opengeolab_app.get_state()['recordedCommandCount']"));
    CHECK(controller.lastPythonOutput().trimmed().endsWith(QStringLiteral("1")));

    controller.runEmbeddedPythonCommandLine(
        QStringLiteral("opengeolab_app.replay_commands()['success']"));
    CHECK(controller.lastPythonOutput().trimmed().contains(QStringLiteral("True")));
}

TEST_CASE("recorded python script exports to a file", "[app][python][export]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.recordSelectionSmokeTest();

    QTemporaryDir temp_dir;
    REQUIRE(temp_dir.isValid());

    const QString export_path = QDir(temp_dir.path()).filePath(QStringLiteral("recorded.py"));
    REQUIRE(controller.exportRecordedScript(export_path));

    QFile exported_file(export_path);
    REQUIRE(exported_file.open(QIODevice::ReadOnly | QIODevice::Text));

    const QString contents = QString::fromUtf8(exported_file.readAll());
    CHECK(contents.contains(QStringLiteral("import opengeolab")));
    CHECK(contents.contains(QStringLiteral("OpenGeoLabPythonBridge")));
    CHECK(contents.contains(QStringLiteral("pickPlaceholderEntity")));
    CHECK(controller.lastSummary().contains(export_path));
}