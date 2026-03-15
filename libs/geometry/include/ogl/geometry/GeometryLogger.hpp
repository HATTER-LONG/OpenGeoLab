/**
 * @file GeometryLogger.hpp
 * @brief Internal logging helpers for the geometry library.
 */

#pragma once

#include <ogl/core/ModuleLogger.hpp>

namespace OGL::Geometry::Internal {

inline auto logger() -> const std::shared_ptr<spdlog::logger>& {
    static auto logger_instance = OGL_CREATE_MODULE_LOGGER("OpenGeoLab.Geometry");
    return logger_instance;
}

} // namespace OGL::Geometry::Internal

#define OGL_GEOMETRY_LOG_WITH_LEVEL(level, ...)                                                    \
    OGL_LOG_WITH_LEVEL(OGL::Geometry::Internal::logger(), (level), __VA_ARGS__)
#define OGL_GEOMETRY_LOG_TRACE(...) OGL_GEOMETRY_LOG_WITH_LEVEL(spdlog::level::trace, __VA_ARGS__)
#define OGL_GEOMETRY_LOG_DEBUG(...) OGL_GEOMETRY_LOG_WITH_LEVEL(spdlog::level::debug, __VA_ARGS__)
#define OGL_GEOMETRY_LOG_INFO(...) OGL_GEOMETRY_LOG_WITH_LEVEL(spdlog::level::info, __VA_ARGS__)
#define OGL_GEOMETRY_LOG_WARN(...) OGL_GEOMETRY_LOG_WITH_LEVEL(spdlog::level::warn, __VA_ARGS__)
#define OGL_GEOMETRY_LOG_ERROR(...) OGL_GEOMETRY_LOG_WITH_LEVEL(spdlog::level::err, __VA_ARGS__)
#define OGL_GEOMETRY_LOG_CRITICAL(...)                                                             \
    OGL_GEOMETRY_LOG_WITH_LEVEL(spdlog::level::critical, __VA_ARGS__)
