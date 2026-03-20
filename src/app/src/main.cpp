/**
 * @file main.cpp
 * @brief Application entry point: initializes Python runtime, ProcessService,
 *        and the QML engine.
 */

#include <pybind11/pybind11.h>

#include <opengeolab/app/process_service.hpp>
#include <opengeolab/python/embedded_python_runtime.hpp>

#include <QApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <filesystem>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    const auto app_dir = QCoreApplication::applicationDirPath().toStdString();
    const auto runtime_dir = std::filesystem::path(app_dir) / "python";
    const auto plugin_dir = std::filesystem::path(app_dir) / "plugins" / "python";

    OpenGeoLab::Python::EmbeddedPythonRuntime python_runtime(app_dir, runtime_dir, plugin_dir);

    OpenGeoLab::App::ProcessService process_service(python_runtime);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("processService", &process_service);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("OpenGeoLab.App", "Main");

    // CRITICAL: Release GIL before entering Qt event loop.
    // scoped_interpreter (inside EmbeddedPythonRuntime) makes the main thread hold the GIL.
    // Worker threads call m_runtime.process() which does gil_scoped_acquire.
    // If we don't release GIL here, worker threads will deadlock waiting for GIL
    // while the main thread is blocked in app.exec().
    // When app.exec() returns, `release` destructor re-acquires GIL automatically,
    // ensuring scoped_interpreter can safely destruct.
    pybind11::gil_scoped_release release;
    return app.exec();
}
