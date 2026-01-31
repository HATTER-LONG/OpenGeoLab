/**
 * @file main.cpp
 * @brief Application entry point
 */
#include "app/render_service.hpp"
#include "service.hpp"
#include "util/logger.hpp"
#include <app/log_service.hpp>
#include <util/qml_spdlog_sink.hpp>

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>

#include <kangaroo/util/stopwatch.hpp>

#ifdef Q_OS_WIN
/**
 * @brief Hint hybrid graphics systems to prefer the discrete GPU
 */
extern "C" {
Q_DECL_EXPORT unsigned long NvOptimusEnablement = 0x00000001; // NOLINT
Q_DECL_EXPORT int AmdPowerXpressRequestHighPerformance = 1;   // NOLINT
}
#endif

namespace {
/**
 * @brief Configure Qt for high-DPI displays
 */
void initQtEnvironment() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
}
} // namespace

auto main(int argc, char** argv) -> int {
    Kangaroo::Util::Stopwatch stopwatch("Total execution time", OpenGeoLab::getLogger());

    initQtEnvironment();
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    OpenGeoLab::App::registerServices();
    QGuiApplication app(argc, argv);

    QIcon app_icon;
    app_icon.addFile(":/opengeolab/resources/icons/toolsbaricon1592x1592.png");
    QGuiApplication::setWindowIcon(app_icon);

    QQmlApplicationEngine engine;

    // Create and register services as context properties
    OpenGeoLab::App::LogService log_service;
    OpenGeoLab::Util::installQmlSpdlogSink(&log_service);
    engine.rootContext()->setContextProperty("LogService", &log_service);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [](QObject* obj, const QUrl& obj_url) {
            if(!obj) {
                LOG_ERROR("Failed to load QML file: {}", obj_url.toString().toStdString());
                QCoreApplication::exit(-1);
            } else {
                LOG_TRACE("QML file loaded successfully: {}", obj_url.toString().toStdString());
            }
        },
        Qt::QueuedConnection);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.load(QUrl("qrc:/opengeolab/resources/qml/Main.qml"));

    return app.exec();
}
