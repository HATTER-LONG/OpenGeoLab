/**
 * @file ComponentDispatcherLogger.hpp
 * @brief Internal logging helpers for the core component dispatcher.
 */

#pragma once

#include <ogl/core/ModuleLogger.hpp>

namespace OGL::Core::Internal {

inline auto componentDispatcherLogger() -> const std::shared_ptr<spdlog::logger>& {
    static auto logger_instance = OGL_CREATE_MODULE_LOGGER("OpenGeoLab.ComponentDispatcher");
    return logger_instance;
}

} // namespace OGL::Core::Internal

#define OGL_COMPONENT_DISPATCH_LOG_WITH_LEVEL(level, ...)                                          \
    OGL_LOG_WITH_LEVEL(OGL::Core::Internal::componentDispatcherLogger(), (level), __VA_ARGS__)
#define OGL_COMPONENT_DISPATCH_LOG_TRACE(...)                                                      \
    OGL_COMPONENT_DISPATCH_LOG_WITH_LEVEL(spdlog::level::trace, __VA_ARGS__)
#define OGL_COMPONENT_DISPATCH_LOG_DEBUG(...)                                                      \
    OGL_COMPONENT_DISPATCH_LOG_WITH_LEVEL(spdlog::level::debug, __VA_ARGS__)
#define OGL_COMPONENT_DISPATCH_LOG_INFO(...)                                                       \
    OGL_COMPONENT_DISPATCH_LOG_WITH_LEVEL(spdlog::level::info, __VA_ARGS__)
#define OGL_COMPONENT_DISPATCH_LOG_WARN(...)                                                       \
    OGL_COMPONENT_DISPATCH_LOG_WITH_LEVEL(spdlog::level::warn, __VA_ARGS__)
#define OGL_COMPONENT_DISPATCH_LOG_ERROR(...)                                                      \
    OGL_COMPONENT_DISPATCH_LOG_WITH_LEVEL(spdlog::level::err, __VA_ARGS__)
#define OGL_COMPONENT_DISPATCH_LOG_CRITICAL(...)                                                   \
    OGL_COMPONENT_DISPATCH_LOG_WITH_LEVEL(spdlog::level::critical, __VA_ARGS__)
