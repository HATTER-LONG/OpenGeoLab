#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../src/OperationLogModel.hpp"

#include <ogl/app/OpenGeoLabController.hpp>
#include <ogl/core/ModuleLogger.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QMetaObject>
#include <QModelIndex>
#include <QTemporaryDir>

#include <spdlog/common.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {

auto ensureCoreApplication() -> QCoreApplication& {
    static int argc = 1;
    static char app_name[] = "opengeolab_embedded_python_smoke_test";
    static char* argv[] = {app_name, nullptr};
    static QCoreApplication app(argc, argv);
    return app;
}

auto recordSelectionForTest(OGL::App::OpenGeoLabController& controller,
                            const std::string& model_name = "RecordReplaySmokeModel")
    -> nlohmann::json {
    return controller.executeCommand({{"module", "selection"},
                                      {"action", "pickEntity"},
                                      {"param",
                                       {{"modelName", model_name},
                                        {"bodyCount", 3},
                                        {"viewportWidth", 1280},
                                        {"viewportHeight", 720},
                                        {"screenX", 412},
                                        {"screenY", 248},
                                        {"requestedBy", "test_embedded_python_runtime"}}}},
                                     "test");
}

} // namespace

TEST_CASE("embedded python runtime drives the command recorder", "[app][python][smoke]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearRecordedCommands();

    controller.runEmbeddedPython(QStringLiteral("import opengeolab_app\n\n"
                                                "result = opengeolab_app.process({\n"
                                                "    \"module\": \"selection\",\n"
                                                "    \"action\": \"pickEntity\",\n"
                                                "    \"param\": {\n"
                                                "        \"modelName\": \"EmbeddedPythonTest\",\n"
                                                "        \"bodyCount\": 2,\n"
                                                "        \"viewportWidth\": 800,\n"
                                                "        \"viewportHeight\": 600,\n"
                                                "        \"screenX\": 160,\n"
                                                "        \"screenY\": 120\n"
                                                "    }\n"
                                                "})\n"
                                                "print(result[\"summary\"])\n"));

    REQUIRE(controller.recordedCommandCount() == 1);
    CHECK(controller.lastResponse().contains(QStringLiteral("\"summary\"")));
    CHECK(controller.lastResponse().contains(QStringLiteral("\"selectionResult\"")));
    CHECK_FALSE(controller.lastResponse().contains(QStringLiteral("\"module\"")));
    CHECK_FALSE(controller.lastResponse().contains(QStringLiteral("\"action\"")));
    CHECK_FALSE(controller.lastResponse().contains(QStringLiteral("\"message\"")));

    const auto state = controller.applicationStateJson();
    CHECK(state.value("lastModule", std::string{}) == "selection");
    CHECK(state.value("lastAction", std::string{}) == "pickEntity");
    CHECK(state.value("recordedCommandCount", 0) == 1);
    CHECK_FALSE(state.value("lastSummary", std::string{}).empty());
}

TEST_CASE("embedded python command line evaluates expressions and statements",
          "[app][python][cli]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearRecordedCommands();

    const auto recorded_response = recordSelectionForTest(controller);
    REQUIRE(recorded_response.value("success", false));
    CHECK(recorded_response.contains("summary"));
    CHECK_FALSE(recorded_response.value("payload", nlohmann::json::object())
                    .contains("equivalentPython"));
    REQUIRE(controller.recordedCommandCount() == 1);

    controller.runEmbeddedPythonCommandLine(QStringLiteral(
        "opengeolab_app.process({'module': 'selection', 'action': 'pickEntity', 'param': "
        "{'modelName': 'EmbeddedPythonCli', 'bodyCount': 2, 'viewportWidth': 800, "
        "'viewportHeight': 600, 'screenX': 64, 'screenY': 32}})['success']"));
    CHECK(controller.lastPythonOutput().trimmed().contains(QStringLiteral("True")));

    controller.runEmbeddedPythonCommandLine(QStringLiteral(
        "result = opengeolab_app.process({'module': 'selection', 'action': 'pickEntity', "
        "'param': {'modelName': 'EmbeddedPythonCliExec', 'bodyCount': 2, 'viewportWidth': 800, "
        "'viewportHeight': 600, 'screenX': 96, 'screenY': 48}})"));
    CHECK(controller.lastPythonOutput().trimmed().contains(
        QStringLiteral("Python command completed without stdout/stderr.")));
    CHECK(controller.recordedCommandCount() == 3);
}

TEST_CASE("recorded python script exports to a file", "[app][python][export]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    const auto recorded_response = recordSelectionForTest(controller);
    REQUIRE(recorded_response.value("success", false));

    QTemporaryDir temp_dir;
    REQUIRE(temp_dir.isValid());

    const QString export_path = QDir(temp_dir.path()).filePath(QStringLiteral("recorded.py"));
    REQUIRE(controller.exportRecordedScript(export_path));

    QFile exported_file(export_path);
    REQUIRE(exported_file.open(QIODevice::ReadOnly | QIODevice::Text));

    const QString contents = QString::fromUtf8(exported_file.readAll());
    CHECK(contents.contains(QStringLiteral("import opengeolab")));
    CHECK(contents.contains(QStringLiteral("OpenGeoLabPythonBridge")));
    CHECK(contents.contains(QStringLiteral("pickEntity")));
    CHECK(controller.lastSummary().contains(export_path));
}

TEST_CASE("controller exposes unread operation logs from module logger output", "[app][logging]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();
    controller.markOperationLogSeen();

    const auto recorded_response = recordSelectionForTest(controller);
    REQUIRE(recorded_response.value("success", false));

    REQUIRE(controller.operationLogModel() != nullptr);
    CHECK(controller.operationLogModel()->rowCount() >= 1);
    CHECK(controller.hasUnreadOperationLogs());

    controller.markOperationLogSeen();
    CHECK_FALSE(controller.hasUnreadOperationLogs());
}

TEST_CASE("operation log service separates runtime level from visible entry filters",
          "[app][logging]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    QObject* service = controller.operationLogService();
    REQUIRE(service != nullptr);

    REQUIRE(service->setProperty("minLevel", 3));
    CHECK(service->property("minLevel").toInt() == 3);

    auto logger = OGL_CREATE_MODULE_LOGGER("OpenGeoLab.TestRuntimeLevel");
    REQUIRE(logger != nullptr);
    CHECK(logger->level() == spdlog::level::warn);

    CHECK(service->property("enabledLevelMask").toInt() == 0x3F);
    REQUIRE(
        QMetaObject::invokeMethod(service, "setLevelEnabled", Q_ARG(int, 2), Q_ARG(bool, false)));
    CHECK((service->property("enabledLevelMask").toInt() & (1 << 2)) == 0);

    REQUIRE(
        QMetaObject::invokeMethod(service, "setLevelEnabled", Q_ARG(int, 2), Q_ARG(bool, true)));
    REQUIRE(service->setProperty("minLevel", 2));
    CHECK(service->property("minLevel").toInt() == 2);
    CHECK(logger->level() == spdlog::level::info);
}

TEST_CASE("geometry create activity logs carry visible source metadata",
          "[app][logging][geometry]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();
    controller.markOperationLogSeen();
    std::vector<double> progress_values;
    QObject::connect(&controller, &OGL::App::OpenGeoLabController::operationProgressChanged,
                     &controller, [&controller, &progress_values]() {
                         progress_values.push_back(controller.operationProgress());
                     });

    REQUIRE(controller.runServiceRequest(QStringLiteral(R"JSON({
  "module": "geometry",
  "action": "createBox",
  "param": {
    "modelName": "MetadataBox",
    "origin": { "x": 1.0, "y": 2.0, "z": 3.0 },
    "dimensions": { "x": 10.0, "y": 20.0, "z": 30.0 }
  }
})JSON")));

    auto* model = controller.operationLogModel();
    REQUIRE(model != nullptr);

    bool found_geometry_logger_entry = false;
    int geometry_logger_entry_count = 0;
    bool found_warn_entry = false;
    bool found_info_entry = false;

    for(int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        const QString source = index.data(OGL::App::OperationLogModel::SourceRole).toString();
        const QString file = index.data(OGL::App::OperationLogModel::FileRole).toString();
        const int line = index.data(OGL::App::OperationLogModel::LineRole).toInt();
        const qint64 thread_id = index.data(OGL::App::OperationLogModel::ThreadIdRole).toLongLong();
        const int level = index.data(OGL::App::OperationLogModel::LevelRole).toInt();

        if(source == QStringLiteral("OpenGeoLab.Geometry")) {
            found_geometry_logger_entry = true;
            ++geometry_logger_entry_count;
            found_warn_entry = found_warn_entry || level == 3;
            found_info_entry = found_info_entry || level == 2;
            CHECK_FALSE(file.isEmpty());
            CHECK(line > 0);
            CHECK(thread_id > 0);
        }
    }

    CHECK(found_geometry_logger_entry);
    CHECK(geometry_logger_entry_count >= 4);
    CHECK(found_warn_entry);
    CHECK(found_info_entry);
    CHECK(std::any_of(progress_values.begin(), progress_values.end(),
                      [](double progress) { return progress > 0.0 && progress < 1.0; }));
    CHECK_FALSE(progress_values.empty());
    CHECK(progress_values.back() == Catch::Approx(1.0));
}

TEST_CASE("async geometry createBox reports progress updates without blocking completion",
          "[app][async][geometry]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    std::vector<double> progress_values;
    std::vector<QString> progress_messages;
    int finished_request_id = -1;
    bool finished_success = false;

    QObject::connect(&controller, &OGL::App::OpenGeoLabController::operationProgressChanged,
                     &controller, [&controller, &progress_values]() {
                         progress_values.push_back(controller.operationProgress());
                     });
    QObject::connect(&controller, &OGL::App::OpenGeoLabController::operationMessageChanged,
                     &controller, [&controller, &progress_messages]() {
                         progress_messages.push_back(controller.operationMessage());
                     });
    QObject::connect(&controller, &OGL::App::OpenGeoLabController::serviceRequestFinished,
                     &controller,
                     [&finished_request_id, &finished_success](int requestId, bool success) {
                         finished_request_id = requestId;
                         finished_success = success;
                     });

    const int request_id = controller.submitServiceRequest(QStringLiteral(R"JSON({
  "module": "geometry",
  "action": "createBox",
  "param": {
    "modelName": "AsyncBox",
    "origin": { "x": 0.0, "y": 0.0, "z": 0.0 },
    "dimensions": { "x": 12.0, "y": 24.0, "z": 36.0 }
  }
})JSON"));

    REQUIRE(request_id > 0);

    QElapsedTimer timer;
    timer.start();
    while(finished_request_id != request_id && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
    }

    REQUIRE(finished_request_id == request_id);
    REQUIRE(finished_success);
    CHECK(std::any_of(progress_values.begin(), progress_values.end(),
                      [](double progress) { return progress > 0.0 && progress < 1.0; }));
    CHECK(
        std::any_of(progress_messages.begin(), progress_messages.end(), [](const QString& message) {
            return message.contains(QStringLiteral("box"), Qt::CaseInsensitive);
        }));
    CHECK(controller.operationState() == QStringLiteral("success"));
}

TEST_CASE("recorder property signals can query cached state without re-locking",
          "[app][recorder][signals]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearRecordedCommands();

    bool count_signal_seen = false;
    bool commands_signal_seen = false;
    QObject::connect(&controller, &OGL::App::OpenGeoLabController::recordedCommandCountChanged,
                     &controller, [&controller, &count_signal_seen]() {
                         count_signal_seen = true;
                         CHECK_NOTHROW(static_cast<void>(controller.recordedCommandCount()));
                     });
    QObject::connect(&controller, &OGL::App::OpenGeoLabController::recordedCommandsChanged,
                     &controller, [&controller, &commands_signal_seen]() {
                         commands_signal_seen = true;
                         CHECK_NOTHROW(static_cast<void>(controller.recordedCommands()));
                     });

    const auto response =
        controller.executeCommand({{"module", "geometry"},
                                   {"action", "createBox"},
                                   {"param",
                                    {{"modelName", "SignalSafeBox"},
                                     {"origin", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
                                     {"dimensions", {{"x", 12.0}, {"y", 24.0}, {"z", 36.0}}},
                                     {"source", "test"}}}},
                                  "test");

    REQUIRE(response.value("success", false));
    CHECK(count_signal_seen);
    CHECK(commands_signal_seen);
    CHECK(controller.recordedCommandCount() == 1);
}

TEST_CASE("controller activity logs carry visible source metadata", "[app][logging][controller]") {
    static_cast<void>(ensureCoreApplication());

    OGL::App::OpenGeoLabController controller;
    controller.clearOperationLog();
    controller.markOperationLogSeen();
    controller.runEmbeddedPythonCommandLine(QStringLiteral("1"));

    auto* model = controller.operationLogModel();
    REQUIRE(model != nullptr);

    bool found_controller_entry = false;
    for(int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        const QString source = index.data(OGL::App::OperationLogModel::SourceRole).toString();
        if(source != QStringLiteral("python.commandLine")) {
            continue;
        }

        found_controller_entry = true;
        CHECK_FALSE(index.data(OGL::App::OperationLogModel::FileRole).toString().isEmpty());
        CHECK(index.data(OGL::App::OperationLogModel::LineRole).toInt() > 0);
        CHECK(index.data(OGL::App::OperationLogModel::ThreadIdRole).toLongLong() > 0);
    }

    CHECK(found_controller_entry);
}
