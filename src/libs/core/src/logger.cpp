#include <kangaroo/util/logger_factory.hpp>
#include <opengeolab/core/logger.hpp>

namespace OpenGeoLab::Core {

static std::shared_ptr<spdlog::logger>& loggerInstance() {
    static auto logger =
        Kangaroo::Util::LoggerFactory::createLogger("OpenGeoLab", spdlog::level::info);
    return logger;
}

spdlog::logger* getLogger() { return loggerInstance().get(); }

std::shared_ptr<spdlog::logger> getLoggerShared() { return loggerInstance(); }

} // namespace OpenGeoLab::Core