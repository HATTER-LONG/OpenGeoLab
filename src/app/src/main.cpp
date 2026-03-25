/**
 * @file main.cpp
 * @brief Application entry point: initializes QML engine, embedded Python
 *        runtime, and layered services.
 */

#include <pybind11/pybind11.h>

#include <opengeolab/app/gl_viewport_item.hpp>
#include <opengeolab/app/main_thread_executor.hpp>
#include <opengeolab/app/notification_service.hpp>
#include <opengeolab/app/progress_tracker.hpp>
#include <opengeolab/app/request_service.hpp>
#include <opengeolab/base/notification_registry.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/python/embedded_python_runtime.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>
#include <QtQml/qqml.h>

#include <filesystem>

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

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QSurfaceFormat fmt;
    fmt.setVersion(4, 5);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    fmt.setSwapInterval(1);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QQuickStyle::setStyle("Fusion");

    const auto app_dir = std::filesystem::path(argv[0]).parent_path();
    const auto runtime_dir = app_dir / "python";
    const auto plugin_dir = app_dir / "plugins";

    OpenGeoLab::Python::EmbeddedPythonRuntime python_runtime(app_dir, runtime_dir, plugin_dir);

    OpenGeoLab::App::ProgressTracker progress_tracker;
    OpenGeoLab::App::NotificationService notification_service;
    OpenGeoLab::App::MainThreadExecutor main_thread_executor;
    OpenGeoLab::Base::NotificationRegistry::setSink(&notification_service);
    OpenGeoLab::App::RequestService request_service(python_runtime, progress_tracker);
    OpenGeoLab::Scene::SceneGraph scene_graph;
    OpenGeoLab::Geometry::GeometryModule geometry_module(scene_graph);
    request_service.setGeometryModule(&geometry_module);

    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "RequestService", &request_service);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "NotificationService",
                                 &notification_service);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "ProgressTracker", &progress_tracker);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "MainThreadExecutor",
                                 &main_thread_executor);

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    const auto wire_scene_graph = [&scene_graph](QObject* object) {
        if(object == nullptr) {
            return;
        }

        auto* viewport = object->findChild<OpenGeoLab::App::GLViewportItem*>();
        if(viewport != nullptr) {
            viewport->setSceneGraph(&scene_graph);
        }
    };

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [&wire_scene_graph](QObject* object, const QUrl&) { wire_scene_graph(object); },
        Qt::QueuedConnection);

    engine.loadFromModule("OpenGeoLab.App", "Main");
    for(QObject* object : engine.rootObjects()) {
        wire_scene_graph(object);
    }

    const pybind11::gil_scoped_release release;
    return app.exec();
}
