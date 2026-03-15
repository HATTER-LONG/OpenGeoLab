/**
 * @file RenderLogger.hpp
 * @brief Internal logging helpers for the render library.
 */

#pragma once

#include <ogl/core/ModuleLogger.hpp>

namespace OGL::Render::Internal {

inline auto logger() -> const std::shared_ptr<spdlog::logger>& {
    static auto logger_instance = OGL_CREATE_MODULE_LOGGER("OpenGeoLab.Render");
    return logger_instance;
}

} // namespace OGL::Render::Internal

#define OGL_RENDER_LOG_WITH_LEVEL(level, ...)                                                      \
    OGL_LOG_WITH_LEVEL(OGL::Render::Internal::logger(), (level), __VA_ARGS__)
#define OGL_RENDER_LOG_TRACE(...) OGL_RENDER_LOG_WITH_LEVEL(spdlog::level::trace, __VA_ARGS__)
#define OGL_RENDER_LOG_DEBUG(...) OGL_RENDER_LOG_WITH_LEVEL(spdlog::level::debug, __VA_ARGS__)
#define OGL_RENDER_LOG_INFO(...) OGL_RENDER_LOG_WITH_LEVEL(spdlog::level::info, __VA_ARGS__)
#define OGL_RENDER_LOG_WARN(...) OGL_RENDER_LOG_WITH_LEVEL(spdlog::level::warn, __VA_ARGS__)
#define OGL_RENDER_LOG_ERROR(...) OGL_RENDER_LOG_WITH_LEVEL(spdlog::level::err, __VA_ARGS__)
#define OGL_RENDER_LOG_CRITICAL(...) OGL_RENDER_LOG_WITH_LEVEL(spdlog::level::critical, __VA_ARGS__)
