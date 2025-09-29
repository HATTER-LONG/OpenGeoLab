#include <greeter/greeter.h>

#include "logger.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <cxxopts.hpp>
#include <iostream>
#include <kangaroo/util/stopwatch.hpp>
#include <string>
#include <unordered_map>

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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
#else
    qputenv("QT_QUICK_CONTROLS_STYLE", "Default");
#endif

#ifdef Q_OS_WINDOWS
    qputenv("QSG_RHI_BACKEND", "d3d12"); // options: d3d11, d3d12, opengl, vulkan
    qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");
#endif
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
    QGuiApplication app(argc, argv);
    // Make sure alpha channel is requested, our special effects on Windows depends on it.
    QQuickWindow::setDefaultAlphaBuffer(true);
    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("OpenGeoLab", "Main");
    return app.exec();
}
