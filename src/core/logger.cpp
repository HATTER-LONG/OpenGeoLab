/**
 * @file logger.cpp
 * @brief Implementation of global logger functions for the OpenGeoLab project
 */

#include <core/logger.hpp>

#include <kangaroo/util/logger_factory.hpp>
#include <spdlog/spdlog.h>

#include <memory>

namespace OpenGeoLab {

std::shared_ptr<spdlog::logger> getLogger() {
    // Meyer's singleton pattern - thread-safe in C++11 and later
    // The logger is created with trace level to capture all log messages
    // Logger name "OpenGeoLab" will appear in log output for easy identification
    static auto logger =
        Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab", spdlog::level::trace);
    return logger;
}

} // namespace OpenGeoLab
