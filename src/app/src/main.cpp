/// @file main.cpp
/// @brief Application entry point for OpenGeoLab.

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
#if defined(_WIN32) && defined(OPENGEOLAB_QT_BIN_DIR)
    // Prepend Qt's own bin directory so the correct DLLs are found before any
    // stale copies that may exist elsewhere on PATH (e.g. Qt Creator's bin).
    qputenv("PATH",
            QDir::toNativeSeparators(OPENGEOLAB_QT_BIN_DIR).toUtf8() + ";" + qgetenv("PATH"));
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("OpenGeoLab");
    app.setOrganizationName("OpenGeoLab");

    QQmlApplicationEngine engine;
    engine.loadFromModule("OpenGeoLab.App", "Main");

    if(engine.rootObjects().isEmpty()) {
        return -1;
    }

    return QGuiApplication::exec();
}
