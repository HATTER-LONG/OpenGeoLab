/**
 * @file main.cpp
 * @brief Main entry point for the OpenGeoLab application
 *
 * This application demonstrates:
 * - Integration of OpenGL rendering with Qt Quick
 * - Command-line argument parsing with cxxopts
 * - Multi-language greeting system
 * - Performance monitoring with Kangaroo utilities
 */

#include <core/logger.hpp>

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

#include "component_example/component_example.hpp"

#ifdef Q_OS_WIN
/**
 * @brief Export symbols to hint hybrid graphics systems to prefer discrete GPU
 *
 * These exports tell NVIDIA Optimus and AMD PowerXpress systems to use the
 * high-performance discrete GPU instead of the integrated GPU for better
 * OpenGL rendering performance.
 */
extern "C" {
Q_DECL_EXPORT unsigned long NvOptimusEnablement = 0x00000001; // NOLINT
Q_DECL_EXPORT int AmdPowerXpressRequestHighPerformance = 1;   // NOLINT
}
#endif

namespace {
/**
 * @brief Initialize Qt environment with high DPI support
 *
 * Configures Qt to properly handle high DPI displays by using the PassThrough
 * scaling policy, which provides smooth scaling on all display resolutions.
 */
void initQtEnvironment() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
}
} // namespace

/**
 * @brief Main application entry point
 *
 * Initializes the Qt application, processes command-line arguments,
 * demonstrates the greeter functionality, and launches the Qt Quick UI.
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return Application exit code (0 for success, non-zero for error)
 */
auto main(int argc, char** argv) -> int {
    // Performance monitoring for total execution time
    Kangaroo::Util::Stopwatch stopwatch("Total execution time", OpenGeoLab::getLogger());

    // Configure command-line options
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

    // Handle help option
    if(result["help"].as<bool>()) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // Handle version option
    if(result["version"].as<bool>()) {
        std::cout << "Greeter, version " << 0.1 << std::endl;
        return 0;
    }

    // Initialize Qt environment for high DPI support
    initQtEnvironment();

    // Set OpenGL as graphics API before creating QGuiApplication
    // This is required for custom OpenGL rendering in Qt Quick
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    // Use Basic style to avoid native Windows style limitations
    // The Basic style provides more consistent cross-platform behavior
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Note: TriangleItem is automatically registered via QML_NAMED_ELEMENT
    // No explicit qmlRegisterType is needed

    // Handle successful QML object creation
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

    // Handle QML object creation failure
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() {
            LOG_ERROR("Failed to create QML objects.");
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    // Load main QML file
    engine.load(QUrl("qrc:/scenegraph/opengeolab/resources/qml/Main.qml"));

    return app.exec();
}
