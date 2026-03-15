/**
 * @file SelectionLogger.hpp
 * @brief Internal logging helpers for the selection library.
 */

#pragma once

#include <ogl/core/ModuleLogger.hpp>

namespace OGL::Selection::Internal {

inline auto logger() -> const std::shared_ptr<spdlog::logger>& {
    static auto logger_instance = OGL_CREATE_MODULE_LOGGER("OpenGeoLab.Selection");
    return logger_instance;
}

} // namespace OGL::Selection::Internal

#define OGL_SELECTION_LOG_WITH_LEVEL(level, ...)                                                   \
    OGL_LOG_WITH_LEVEL(OGL::Selection::Internal::logger(), (level), __VA_ARGS__)
#define OGL_SELECTION_LOG_TRACE(...) OGL_SELECTION_LOG_WITH_LEVEL(spdlog::level::trace, __VA_ARGS__)
#define OGL_SELECTION_LOG_DEBUG(...) OGL_SELECTION_LOG_WITH_LEVEL(spdlog::level::debug, __VA_ARGS__)
#define OGL_SELECTION_LOG_INFO(...) OGL_SELECTION_LOG_WITH_LEVEL(spdlog::level::info, __VA_ARGS__)
#define OGL_SELECTION_LOG_WARN(...) OGL_SELECTION_LOG_WITH_LEVEL(spdlog::level::warn, __VA_ARGS__)
#define OGL_SELECTION_LOG_ERROR(...) OGL_SELECTION_LOG_WITH_LEVEL(spdlog::level::err, __VA_ARGS__)
#define OGL_SELECTION_LOG_CRITICAL(...)                                                            \
    OGL_SELECTION_LOG_WITH_LEVEL(spdlog::level::critical, __VA_ARGS__)
