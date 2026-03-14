#include <ogl/app/OpenGeoLabController.hpp>

#include <kangaroo/util/logger_factory.hpp>

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

    auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab");
    logger->info("Starting OpenGeoLab modular application shell");

    OGL::App::OpenGeoLabController controller;
    QQmlApplicationEngine engine;

    engine.setInitialProperties(
        {{QStringLiteral("appController"), QVariant::fromValue(&controller)}});
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    logger->info("Added standard QML import path qrc:/qt/qml");

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("OpenGeoLab"), QStringLiteral("Main"));
    if(engine.rootObjects().isEmpty()) {
        logger->error("Failed to load OpenGeoLab::Main from the QML module");
        return -1;
    }

    logger->info("OpenGeoLab modular UI loaded successfully");
    return app.exec();
}