/**
 * @file SceneLogger.hpp
 * @brief Internal logging helpers for the scene library.
 */

#pragma once

#include <ogl/core/ModuleLogger.hpp>

namespace OGL::Scene::Internal {

inline auto logger() -> const std::shared_ptr<spdlog::logger>& {
    static auto logger_instance = OGL_CREATE_MODULE_LOGGER("OpenGeoLab.Scene");
    return logger_instance;
}

} // namespace OGL::Scene::Internal

#define OGL_SCENE_LOG_WITH_LEVEL(level, ...)                                                       \
    OGL_LOG_WITH_LEVEL(OGL::Scene::Internal::logger(), (level), __VA_ARGS__)
#define OGL_SCENE_LOG_TRACE(...) OGL_SCENE_LOG_WITH_LEVEL(spdlog::level::trace, __VA_ARGS__)
#define OGL_SCENE_LOG_DEBUG(...) OGL_SCENE_LOG_WITH_LEVEL(spdlog::level::debug, __VA_ARGS__)
#define OGL_SCENE_LOG_INFO(...) OGL_SCENE_LOG_WITH_LEVEL(spdlog::level::info, __VA_ARGS__)
#define OGL_SCENE_LOG_WARN(...) OGL_SCENE_LOG_WITH_LEVEL(spdlog::level::warn, __VA_ARGS__)
#define OGL_SCENE_LOG_ERROR(...) OGL_SCENE_LOG_WITH_LEVEL(spdlog::level::err, __VA_ARGS__)
#define OGL_SCENE_LOG_CRITICAL(...) OGL_SCENE_LOG_WITH_LEVEL(spdlog::level::critical, __VA_ARGS__)
