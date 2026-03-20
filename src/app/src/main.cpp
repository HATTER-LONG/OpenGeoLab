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
#include <QQuickStyle>

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#if defined(_WIN32) && defined(OPENGEOLAB_QT_BIN_DIR)
    // Development convenience: insert the Qt bin directory into DLL search
    // order so that Qt plugin transitive dependencies (e.g. Qt6QuickLayouts)
    // resolve from the project's Qt rather than a different Qt version that
    // might appear on PATH (e.g. Qt Creator ships its own Qt 6.x).
    // This is a compile-time path; for deployment use windeployqt instead.
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
