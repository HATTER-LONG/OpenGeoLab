#include <ogl/app/OpenGeoLabController.hpp>

#include <kangaroo/util/logger_factory.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QString>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("OpenGeoLab");
    QCoreApplication::setOrganizationName("OpenGeoLab");

    auto logger = Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab");
    logger->info("Starting OpenGeoLab modular application shell");

    ogl::app::OpenGeoLabController controller;
    QQmlApplicationEngine engine;
    const QString build_qml_import_path =
        QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    engine.rootContext()->setContextProperty(QStringLiteral("openGeoLabController"), &controller);
    engine.addImportPath(build_qml_import_path);
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    logger->info("Added QML import path {}", build_qml_import_path.toStdString());

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