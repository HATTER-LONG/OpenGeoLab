/**
 * @file main.cpp
 * @brief Application entry point: initializes QML engine, embedded Python
 *        runtime, and ProcessService.
 */

#include <pybind11/pybind11.h>

#include <opengeolab/app/process_service.hpp>
#include <opengeolab/python/embedded_python_runtime.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

auto main(int argc, char* argv[]) -> int {
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
    OpenGeoLab::App::ProcessService process_service(python_runtime);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("processService", &process_service);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("OpenGeoLab.App", "Main");

    const pybind11::gil_scoped_release release;
    return app.exec();
}
