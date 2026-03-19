#include <opengeolab/app/AppController.hpp>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("OpenGeoLab"));
    QCoreApplication::setApplicationName(QStringLiteral("OpenGeoLabSkeleton"));

    QQmlApplicationEngine engine;
    OpenGeoLab::App::AppController controller;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &application,
        []() {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );

    engine.loadFromModule(QStringLiteral("OpenGeoLab.App"), QStringLiteral("Main"));
    return application.exec();
}
