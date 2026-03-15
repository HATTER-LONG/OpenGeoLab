#include <ogl/app/AppLogger.hpp>
#include <ogl/app/OpenGeoLabController.hpp>
#include <ogl/app/UiSettingsController.hpp>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QString>
#include <QVariant>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("OpenGeoLab");
    QCoreApplication::setOrganizationName("OpenGeoLab");

    OGL::App::OpenGeoLabController controller;
    OGL_APP_LOG_INFO("Starting OpenGeoLab modular application shell");
    QQmlApplicationEngine engine;
    OGL::App::UiSettingsController uiSettingsController(engine);

    engine.setInitialProperties(
        {{QStringLiteral("appController"), QVariant::fromValue(&controller)},
         {QStringLiteral("uiSettingsController"), QVariant::fromValue(&uiSettingsController)}});
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    OGL_APP_LOG_INFO("Added standard QML import path qrc:/qt/qml");

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("OpenGeoLab"), QStringLiteral("Main"));
    if(engine.rootObjects().isEmpty()) {
        OGL_APP_LOG_ERROR("Failed to load OpenGeoLab::Main from the QML module");
        return -1;
    }

    OGL_APP_LOG_INFO("OpenGeoLab modular UI loaded successfully");
    return app.exec();
}
