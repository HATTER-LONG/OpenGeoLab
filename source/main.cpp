#include <greeter/greeter.h>

#include "logger.hpp"
#include "triangle.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <cxxopts.hpp>
#include <iostream>
#include <kangaroo/util/stopwatch.hpp>
#include <string>
#include <unordered_map>

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickWindow>

#ifdef Q_OS_WIN
// Indicates to hybrid graphics systems to prefer the discrete part by default.
extern "C" {
Q_DECL_EXPORT unsigned long NvOptimusEnablement = 0x00000001; // NOLINT
Q_DECL_EXPORT int AmdPowerXpressRequestHighPerformance = 1;   // NOLINT
}
#endif

namespace {
void initQtEnvironment() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
}
} // namespace

auto main(int argc, char** argv) -> int {
    Kangaroo::Util::Stopwatch stopwatch("Total execution time", OpenGeoLab::getLogger());
    const std::unordered_map<std::string, greeter::LanguageCode> languages{
        {"en", greeter::LanguageCode::EN},
        {"de", greeter::LanguageCode::DE},
        {"es", greeter::LanguageCode::ES},
        {"fr", greeter::LanguageCode::FR},
    };

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

    auto lang_it = languages.find(language);
    if(lang_it == languages.end()) {
        std::cerr << "unknown language code: " << language << std::endl;
        return 1;
    }
    greeter::Greeter greeter(name);
    LOG_INFO("{}", greeter.greet(lang_it->second));

    initQtEnvironment();

    // Set OpenGL as graphics API before creating QGuiApplication
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    // 设置使用 Basic 样式,避免 Windows 原生样式的限制
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // TriangleItem is automatically registered via QML_NAMED_ELEMENT
    // No explicit qmlRegisterType needed

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

    engine.load(QUrl("qrc:/scenegraph/opengeolab/source/Main.qml"));

    return app.exec();
}
