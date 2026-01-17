/**
 * @file logger.cpp
 * @brief Implementation of the global logger accessor
 */

#include <util/logger.hpp>

#include <kangaroo/util/logger_factory.hpp>
#include <spdlog/spdlog.h>

#include <memory>

namespace OpenGeoLab {

std::shared_ptr<spdlog::logger> getLogger() {
    static auto logger =
        Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab", spdlog::level::info);
    return logger;
}

} // namespace OpenGeoLab
