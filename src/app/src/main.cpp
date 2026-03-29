/**
 * @file main.cpp
 * @brief Application entry point for OpenGeoLab.
 */

#include <pybind11/pybind11.h>

#include "opengeolab/app/log_event_model.h"
#include "opengeolab/app/log_filter_proxy_model.h"
#include "opengeolab/app/module_data_notifier.h"
#include "opengeolab/app/request_service.h"
#include "scene_bridge.h"

#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/command/module_registry.hpp>
#include <opengeolab/core/logger.hpp>
#include <opengeolab/python_embed/embedded_python_runtime.hpp>
#include <opengeolab/render/viewport_item.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtQml/qqml.h>

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    // QQuickFramebufferObject requires OpenGL; Qt 6.x defaults to D3D11 on Windows.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

#if defined(_WIN32) && defined(OPENGEOLAB_QT_BIN_DIR)
    {
        const auto qt_bin = std::filesystem::path(OPENGEOLAB_QT_BIN_DIR);
        if(std::filesystem::is_directory(qt_bin)) {
            SetDllDirectoryW(qt_bin.wstring().c_str());
        }
    }
#endif

    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QQuickStyle::setStyle("Fusion");
    app.setApplicationName("OpenGeoLab");
    app.setOrganizationName("OpenGeoLab");

    // Use Qt's applicationDirPath for a reliable absolute path regardless of
    // how the executable was launched (double-click, debugger, terminal, etc.).
    const auto app_dir = std::filesystem::path(QApplication::applicationDirPath().toStdString());
    const auto runtime_dir = app_dir / "python";
    const auto plugin_dir = app_dir / "plugins";

    OpenGeoLab::Command::registerBuiltinModules(g_PluginComponentFactory);
    OpenGeoLab::Command::CommandDispatcher dispatcher(g_PluginComponentFactory);

    OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime python_runtime(app_dir, runtime_dir, plugin_dir);

    OpenGeoLab::App::RequestService request_service(dispatcher, python_runtime);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "RequestService", &request_service);

    OpenGeoLab::App::ModuleDataNotifier module_notifier(dispatcher);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "ModuleDataNotifier",
                                 &module_notifier);

    // Scene graph and data-flow bridge
    OpenGeoLab::Scene::SceneGraph scene_graph;
    OpenGeoLab::App::SceneBridge scene_bridge(dispatcher, scene_graph);

    QObject::connect(&module_notifier, &OpenGeoLab::App::ModuleDataNotifier::geometryDataChanged,
                     &scene_bridge, &OpenGeoLab::App::SceneBridge::onGeometryDataChanged);
    QObject::connect(&module_notifier, &OpenGeoLab::App::ModuleDataNotifier::meshDataChanged,
                     &scene_bridge, &OpenGeoLab::App::SceneBridge::onMeshDataChanged);

    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "SceneBridge", &scene_bridge);

    // Register ViewportItem QML type
    qmlRegisterType<OpenGeoLab::Render::ViewportItem>("OpenGeoLab.Render", 1, 0, "ViewportItem");

    OpenGeoLab::App::LogEventModel log_event_model;
    log_event_model.installSink(OpenGeoLab::Core::getLoggerShared());
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "LogEventModel", &log_event_model);
    qmlRegisterType<OpenGeoLab::App::LogFilterProxyModel>("OpenGeoLab.Services", 1, 0,
                                                          "LogFilterProxyModel");

    QQmlApplicationEngine engine;
    engine.loadFromModule("OpenGeoLab.App", "Main");

    if(engine.rootObjects().isEmpty()) {
        return -1;
    }

    // Inject SceneGraph into all ViewportItem instances created by QML
    for(QObject* root : engine.rootObjects()) {
        auto viewports = root->findChildren<OpenGeoLab::Render::ViewportItem*>();
        for(auto* vp : viewports) {
            vp->setSceneGraph(&scene_graph);
            QObject::connect(&scene_bridge, &OpenGeoLab::App::SceneBridge::sceneUpdated, vp,
                             &QQuickItem::update);
        }
    }

    // Release GIL before entering the event loop. EmbeddedPythonRuntime::process()
    // re-acquires GIL internally. This is required for PySide6 invoke_ui — Qt
    // widget creation must happen on the main thread with no GIL held.
    const pybind11::gil_scoped_release release;
    return QApplication::exec();
}
