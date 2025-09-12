#pragma once

#include <spdlog/spdlog.h>

/**
 * @file logger.hpp
 * @brief Global logger instance and convenience macros for the GLRender project.
 *
 * This file provides a global logger instance for the GLRender project and
 * convenient logging macros that can be used throughout the codebase.
 */

namespace OpenGeoLab {
/**
 * @brief Get the global logger instance for the OpenGeoLab project.
 *
 * This function returns a singleton logger instance that is initialized with trace level.
 * The logger is created on first access using the Meyer's singleton pattern,
 * which avoids static initialization order issues and ensures only one instance exists.
 *
 * @return Shared pointer to the global logger instance
 */
std::shared_ptr<spdlog::logger> getLogger();
} // namespace OpenGeoLab

// Convenience logging macros using the global GLRender logger
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(OpenGeoLab::getLogger(), __VA_ARGS__)
