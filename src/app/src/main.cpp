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

#include <opengeolab/app/gl_viewport.hpp>
#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/command/module_registry.hpp>
#include <opengeolab/core/logger.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/python_embed/embedded_python_runtime.hpp>
#include <opengeolab/scene/geometry_scene_bridge.hpp>
#include <opengeolab/scene/scene_module.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QtQml/qqml.h>

#include <filesystem>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    // Force OpenGL backend — QQuickFramebufferObject requires OpenGL, not D3D/Vulkan.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    {
        QSurfaceFormat fmt;
        fmt.setVersion(3, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
        fmt.setDepthBufferSize(24);
        fmt.setStencilBufferSize(8);
        QSurfaceFormat::setDefaultFormat(fmt);
    }

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

    // Use Qt's applicationDirPath for a reliable absolute path regardless of
    // how the executable was launched (double-click, debugger, terminal, etc.).
    const auto app_dir = std::filesystem::path(QApplication::applicationDirPath().toStdString());
    const auto runtime_dir = app_dir / "python";
    const auto plugin_dir = app_dir / "plugins";

    OpenGeoLab::Command::registerBuiltinModules(g_PluginComponentFactory);
    OpenGeoLab::Command::CommandDispatcher dispatcher(g_PluginComponentFactory);

    // ── Scene infrastructure ──────────────────────────────────────────
    auto scene_module_ptr = dispatcher.findModule("scene");
    auto* scene_module = dynamic_cast<OpenGeoLab::Scene::SceneModule*>(scene_module_ptr.get());

    OpenGeoLab::Scene::TopologyIndex topology_index;

    auto geometry_module_base = dispatcher.findModule("geometry");
    auto* geometry_module =
        dynamic_cast<OpenGeoLab::Geometry::GeometryModule*>(geometry_module_base.get());

    std::unique_ptr<OpenGeoLab::Scene::GeometrySceneBridge> scene_bridge;
    if(geometry_module != nullptr && scene_module != nullptr) {
        scene_bridge = std::make_unique<OpenGeoLab::Scene::GeometrySceneBridge>(
            scene_module->sceneGraph(), geometry_module->shapeStore(), topology_index);
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
    }
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "SelectionService",
                                 &selection_service);

    QQmlApplicationEngine engine;
    engine.loadFromModule("OpenGeoLab.App", "Main");

    if(engine.rootObjects().isEmpty()) {
        return -1;
    }

    // ── Wire GLViewport to the SceneGraph ─────────────────────────────
    auto* viewport = engine.rootObjects().first()->findChild<OpenGeoLab::App::GLViewport*>();
    if(viewport != nullptr && scene_module != nullptr) {
        viewport->setSceneGraph(&scene_module->sceneGraph());

        QObject::connect(&module_notifier,
                         &OpenGeoLab::App::ModuleDataNotifier::geometryDataChanged, viewport,
                         [viewport]() { viewport->update(); });

        QObject::connect(&module_notifier, &OpenGeoLab::App::ModuleDataNotifier::sceneDataChanged,
                         viewport, [viewport]() { viewport->update(); });
    }

    // Release GIL before entering the event loop. EmbeddedPythonRuntime::process()
    // re-acquires GIL internally. This is required for PySide6 invoke_ui — Qt
    // widget creation must happen on the main thread with no GIL held.
    const pybind11::gil_scoped_release release;
    return QApplication::exec();
}
