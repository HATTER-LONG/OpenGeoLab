/**
 * @file main.cpp
 * @brief Application entry point
 */
#include "service.hpp"
#include <util/logger.hpp>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>

#include <cxxopts.hpp>
#include <kangaroo/util/stopwatch.hpp>

#include <iostream>
#include <string>

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

    cxxopts::Options options(*argv, "A program to welcome the world!");

    std::string language;
    std::string name;

    // clang-format off
    options.add_options()
        ("h,help", "Show help")
        ("v,version", "Print the current version number")
        ("n,name", "Name to greet", cxxopts::value(name)->default_value("World"))
        ("l,lang", "Language code to use", cxxopts::value(language)->default_value("en"))
    ;
    // clang-format on

    auto result = options.parse(argc, argv);

    if(result["help"].as<bool>()) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if(result["version"].as<bool>()) {
        std::cout << "Greeter, version " << 0.1 << std::endl;
        return 0;
    }

    initQtEnvironment();
    OpenGeoLab::App::registerServices();
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [](QObject* obj, const QUrl& obj_url) {
            if(!obj) {
                LOG_ERROR("Failed to load QML file: {}", obj_url.toString().toStdString());
                QCoreApplication::exit(-1);
            } else {
                LOG_INFO("QML file loaded successfully: {}", obj_url.toString().toStdString());
            }
        },
        Qt::QueuedConnection);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() {
            LOG_ERROR("Failed to create QML objects.");
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(QUrl("qrc:/opengeolab/resources/qml/Main.qml"));

    return app.exec();
}
