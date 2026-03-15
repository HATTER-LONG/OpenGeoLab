/**
 * @file AppLogger.hpp
 * @brief Internal logging helpers for the OpenGeoLab application shell.
 */

#pragma once

#include <ogl/core/ModuleLogger.hpp>

namespace OGL::App::Internal {

inline auto logger() -> const std::shared_ptr<spdlog::logger>& {
    static auto logger_instance = OGL_CREATE_MODULE_LOGGER("OpenGeoLab.App");
    return logger_instance;
}

} // namespace OGL::App::Internal

#define OGL_APP_LOG_WITH_LEVEL(level, ...)                                                         \
    OGL_LOG_WITH_LEVEL(OGL::App::Internal::logger(), (level), __VA_ARGS__)
#define OGL_APP_LOG_TRACE(...) OGL_APP_LOG_WITH_LEVEL(spdlog::level::trace, __VA_ARGS__)
#define OGL_APP_LOG_DEBUG(...) OGL_APP_LOG_WITH_LEVEL(spdlog::level::debug, __VA_ARGS__)
#define OGL_APP_LOG_INFO(...) OGL_APP_LOG_WITH_LEVEL(spdlog::level::info, __VA_ARGS__)
#define OGL_APP_LOG_WARN(...) OGL_APP_LOG_WITH_LEVEL(spdlog::level::warn, __VA_ARGS__)
#define OGL_APP_LOG_ERROR(...) OGL_APP_LOG_WITH_LEVEL(spdlog::level::err, __VA_ARGS__)
#define OGL_APP_LOG_CRITICAL(...) OGL_APP_LOG_WITH_LEVEL(spdlog::level::critical, __VA_ARGS__)
