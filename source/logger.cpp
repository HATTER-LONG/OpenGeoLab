#include "logger.hpp"
#include "kangaroo/util/logger_factory.hpp"
#include <memory>
#include <spdlog/spdlog.h>

/**
 * @file logger.cpp
 * @brief Implementation of global logger functions for the GLRender project.
 */

namespace OpenGeoLab {

std::shared_ptr<spdlog::logger> getLogger() {
    static auto logger =
        Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab", spdlog::level::trace);
    return logger;
}

} // namespace OpenGeoLab
