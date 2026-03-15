/**
 * @file ModuleLogger.hpp
 * @brief Shared logger helpers for OpenGeoLab modules and UI sink fan-out.
 */

#pragma once

#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <mutex>
#include <vector>

namespace OGL::Core {

inline auto moduleLoggerMutex() -> std::mutex& {
    static std::mutex mutex;
    return mutex;
}

inline auto additionalLoggerSinks() -> std::vector<spdlog::sink_ptr>& {
    static std::vector<spdlog::sink_ptr> sinks;
    return sinks;
}

inline auto moduleLoggerLevel() -> spdlog::level::level_enum& {
    static auto level = spdlog::level::info;
    return level;
}

inline auto currentModuleLoggerLevel() -> spdlog::level::level_enum {
    std::scoped_lock lock(moduleLoggerMutex());
    return moduleLoggerLevel();
}

inline void attachSinkToLogger(const std::shared_ptr<spdlog::logger>& logger,
                               const spdlog::sink_ptr& sink) {
    if(!logger || !sink) {
        return;
    }

    auto& logger_sinks = logger->sinks();
    const bool already_attached =
        std::any_of(logger_sinks.begin(), logger_sinks.end(),
                    [&](const auto& existing_sink) { return existing_sink.get() == sink.get(); });
    if(!already_attached) {
        logger_sinks.push_back(sink);
    }
}

inline void registerAdditionalLoggerSink(const spdlog::sink_ptr& sink) {
    if(!sink) {
        return;
    }

    {
        std::scoped_lock lock(moduleLoggerMutex());
        auto& sinks = additionalLoggerSinks();
        const bool already_registered =
            std::any_of(sinks.begin(), sinks.end(), [&](const auto& existing_sink) {
                return existing_sink.get() == sink.get();
            });
        if(!already_registered) {
            sinks.push_back(sink);
        }
    }

    spdlog::apply_all(
        [&](const std::shared_ptr<spdlog::logger>& logger) { attachSinkToLogger(logger, sink); });
}

inline void setModuleLoggerLevel(spdlog::level::level_enum level) {
    {
        std::scoped_lock lock(moduleLoggerMutex());
        if(moduleLoggerLevel() == level) {
            return;
        }
        moduleLoggerLevel() = level;
    }

    spdlog::apply_all([level](const std::shared_ptr<spdlog::logger>& logger) {
        if(logger) {
            logger->set_level(level);
        }
    });
}

inline auto createModuleLogger(const std::string& module_name,
                               spdlog::level::level_enum level = spdlog::level::info)
    -> std::shared_ptr<spdlog::logger> {
    if(auto existing_logger = spdlog::get(module_name)) {
        const auto global_level = currentModuleLoggerLevel();
        const auto effective_level = static_cast<spdlog::level::level_enum>(
            std::max(static_cast<int>(existing_logger->level()), static_cast<int>(global_level)));
        existing_logger->set_level(effective_level);
        return existing_logger;
    }

    std::vector<spdlog::sink_ptr> sinks;
    auto effective_level = level;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-8l%$] [%n] [%s:%#] [tid:%t] %v");
    sinks.push_back(console_sink);

    {
        std::scoped_lock lock(moduleLoggerMutex());
        effective_level = static_cast<spdlog::level::level_enum>(
            std::max(static_cast<int>(level), static_cast<int>(moduleLoggerLevel())));
        const auto& extra_sinks = additionalLoggerSinks();
        sinks.insert(sinks.end(), extra_sinks.begin(), extra_sinks.end());
    }

    auto logger = std::make_shared<spdlog::logger>(module_name, sinks.begin(), sinks.end());
    logger->set_level(effective_level);
    logger->flush_on(spdlog::level::warn);

    try {
        spdlog::register_logger(logger);
        return logger;
    } catch(const spdlog::spdlog_ex&) {
        if(auto existing_logger = spdlog::get(module_name)) {
            return existing_logger;
        }
        throw;
    }
}

} // namespace OGL::Core

#define OGL_CREATE_MODULE_LOGGER(module_name) OGL::Core::createModuleLogger((module_name))
#define OGL_LOG_WITH_LEVEL(logger, level, ...)                                                     \
    do {                                                                                           \
        const auto& ogl_logger__ = (logger);                                                       \
        if(ogl_logger__) {                                                                         \
            ogl_logger__->log(spdlog::source_loc{__FILE__, __LINE__, ""}, (level), __VA_ARGS__);   \
        }                                                                                          \
    } while(false)
#define OGL_LOG_TRACE(logger, ...) OGL_LOG_WITH_LEVEL((logger), spdlog::level::trace, __VA_ARGS__)
#define OGL_LOG_DEBUG(logger, ...) OGL_LOG_WITH_LEVEL((logger), spdlog::level::debug, __VA_ARGS__)
#define OGL_LOG_INFO(logger, ...) OGL_LOG_WITH_LEVEL((logger), spdlog::level::info, __VA_ARGS__)
#define OGL_LOG_WARN(logger, ...) OGL_LOG_WITH_LEVEL((logger), spdlog::level::warn, __VA_ARGS__)
#define OGL_LOG_ERROR(logger, ...) OGL_LOG_WITH_LEVEL((logger), spdlog::level::err, __VA_ARGS__)
#define OGL_LOG_CRITICAL(logger, ...)                                                              \
    OGL_LOG_WITH_LEVEL((logger), spdlog::level::critical, __VA_ARGS__)
