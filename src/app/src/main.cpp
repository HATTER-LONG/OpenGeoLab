/// @file main.cpp
/// @brief Application entry point for OpenGeoLab.

#include <pybind11/pybind11.h>

#include "opengeolab/app/request_service.h"

#include <opengeolab/python_embed/embedded_python_runtime.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
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

    OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime python_runtime(app_dir, runtime_dir, plugin_dir);

    OpenGeoLab::App::RequestService request_service(python_runtime);
    qmlRegisterSingletonInstance("OpenGeoLab.Services", 1, 0, "RequestService", &request_service);

    QQmlApplicationEngine engine;
    engine.loadFromModule("OpenGeoLab.App", "Main");

    if(engine.rootObjects().isEmpty()) {
        return -1;
    }

    // Release GIL before entering the event loop. EmbeddedPythonRuntime::process()
    // re-acquires GIL internally. This is required for PySide6 invoke_ui — Qt
    // widget creation must happen on the main thread with no GIL held.
    const pybind11::gil_scoped_release release;
    return QApplication::exec();
}
