/**
 * @file main.cpp
 * @brief Application entry point for OpenGeoLab.
 */

#include <pybind11/pybind11.h>

#include "opengeolab/app/log_event_model.hpp"
#include "opengeolab/app/log_filter_proxy_model.hpp"
#include "opengeolab/app/module_data_notifier.hpp"
#include "opengeolab/app/request_service.hpp"
#include "opengeolab/app/selection_service.hpp"

#include <opengeolab/app/rhi_viewport.hpp>
#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/command/module_registry.hpp>
#include <opengeolab/core/logger.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/mesh/mesh_module.hpp>
#include <opengeolab/python_embed/embedded_python_runtime.hpp>
#include <opengeolab/scene/scene_module.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QtQml/qqml.h>

#include <filesystem>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#if defined(_WIN32) && defined(OPENGEOLAB_QT_BIN_DIR)
    {
        const auto qt_bin = std::filesystem::path(OPENGEOLAB_QT_BIN_DIR);
        if(std::filesystem::is_directory(qt_bin)) {
            SetDllDirectoryW(qt_bin.wstring().c_str());
        }
    }
#endif

    const QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QQuickStyle::setStyle("Fusion");
    app.setApplicationName("OpenGeoLab");
    app.setOrganizationName("OpenGeoLab");

    QCommandLineParser parser;
    parser.setApplicationDescription("OpenGeoLab — open-source CAD/CAE platform");
    parser.addHelpOption();

    const QCommandLineOption start_http_server_opt(
        QStringLiteral("start-http-server"),
        QStringLiteral("Auto-launch http_server_plugin and start the HTTP server."));
    parser.addOption(start_http_server_opt);
    parser.process(app);

    const bool auto_start_http = parser.isSet(start_http_server_opt);

    // Use Qt's applicationDirPath for a reliable absolute path regardless of
    // how the executable was launched (double-click, debugger, terminal, etc.).
    const auto app_dir = std::filesystem::path(QApplication::applicationDirPath().toStdString());
    const auto runtime_dir = app_dir / "python";
    const auto plugin_dir = app_dir / "plugins";

    OpenGeoLab::Command::registerBuiltinModules(g_PluginComponentFactory);
    OpenGeoLab::Command::CommandDispatcher dispatcher(g_PluginComponentFactory);

    // ── Eagerly cache modules and wire cross-module connections ────────
    // Order matters: leaf modules first, then dependents.
    (void)dispatcher.findModule("io");
    auto geo_ptr = dispatcher.findModule("geometry");
    auto scene_ptr = dispatcher.findModule("scene");
    auto mesh_ptr = dispatcher.findModule("mesh");

    auto* geo_module = dynamic_cast<OpenGeoLab::Geometry::GeometryModule*>(geo_ptr.get());
    auto* scene_module = dynamic_cast<OpenGeoLab::Scene::SceneModule*>(scene_ptr.get());
    auto* mesh_module = dynamic_cast<OpenGeoLab::Mesh::MeshModule*>(mesh_ptr.get());

    if(geo_module != nullptr && scene_module != nullptr) {
        scene_module->initBridge(geo_module->shapeStore());
    }
    if(mesh_module != nullptr && scene_module != nullptr && geo_module != nullptr) {
        mesh_module->initBridge(scene_module->sceneGraph(), geo_module->shapeStore());
    }

    OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime python_runtime(app_dir, runtime_dir, plugin_dir);

    OpenGeoLab::App::RequestService request_service(dispatcher, python_runtime);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "RequestService", &request_service);

    OpenGeoLab::App::ModuleDataNotifier module_notifier(dispatcher);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "ModuleDataNotifier",
                                 &module_notifier);

    OpenGeoLab::App::LogEventModel log_event_model;
    log_event_model.installSink(OpenGeoLab::Core::getLoggerShared());
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "LogEventModel", &log_event_model);
    qmlRegisterType<OpenGeoLab::App::LogFilterProxyModel>("OpenGeoLab.Services", 1, 0,
                                                          "LogFilterProxyModel");

    OpenGeoLab::App::SelectionService selection_service;
    if(scene_module != nullptr) {
        selection_service.setSelectionState(&scene_module->sceneGraph().selectionState());
        selection_service.setLabelManager(&scene_module->sceneGraph().labelManager());
    }
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "SelectionService",
                                 &selection_service);

    QQmlApplicationEngine engine;
    engine.loadFromModule("OpenGeoLab.App", "Main");

    if(engine.rootObjects().isEmpty()) {
        return -1;
    }

    // ── Wire RhiViewport to the SceneGraph ─────────────────────────────
    auto* viewport = engine.rootObjects().first()->findChild<OpenGeoLab::App::RhiViewport*>();
    if(viewport != nullptr && scene_module != nullptr) {
        viewport->setSceneGraph(&scene_module->sceneGraph());

        QObject::connect(&module_notifier,
                         &OpenGeoLab::App::ModuleDataNotifier::geometryDataChanged, viewport,
                         [viewport]() { viewport->update(); });

        QObject::connect(&module_notifier, &OpenGeoLab::App::ModuleDataNotifier::sceneDataChanged,
                         viewport, [viewport]() { viewport->update(); });

        QObject::connect(&module_notifier,
                         &OpenGeoLab::App::ModuleDataNotifier::viewportRefreshNeeded, viewport,
                         [viewport]() { viewport->update(); });
    }

    if(auto_start_http) {
        QTimer::singleShot(0, &request_service, [&request_service]() {
            request_service.executeOnMainThread(
                QStringLiteral(R"({"module":"plugins","action":"invoke_ui",)"
                               R"("param":{"pluginName":"http_server_plugin",)"
                               R"("autoStart":true},"mute":true})"));
        });
    }

    // Release GIL before entering the event loop. EmbeddedPythonRuntime::process()
    // re-acquires GIL internally. This is required for PySide6 invoke_ui — Qt
    // widget creation must happen on the main thread with no GIL held.
    const pybind11::gil_scoped_release release;
    return QApplication::exec();
}
