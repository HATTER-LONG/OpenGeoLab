/**
 * @file logger.hpp
 * @brief Global logger access and convenience logging macros
 */

#pragma once

#include <opengeolab/core/core_export.hpp>

#include <spdlog/spdlog.h>

#include <memory>

namespace OpenGeoLab::Core {

/**
 * @brief Get the global logger instance (non-owning)
 *
 * The logger is created on first use and shared across the process.
 *
 * @return Pointer to the global logger
 * @note Initialization is thread-safe in C++11 and later
 */
[[nodiscard]] OPENGEOLAB_CORE_EXPORT spdlog::logger* getLogger();

/**
 * @brief Get the global logger as shared_ptr for APIs requiring ownership
 *
 * Returns a copy of the internal shared_ptr, incrementing the reference count.
 * Prefer getLogger() for high-frequency logging paths.
 *
 * @return Shared pointer to the global logger
 */
[[nodiscard]] OPENGEOLAB_CORE_EXPORT std::shared_ptr<spdlog::logger> getLoggerShared();

} // namespace OpenGeoLab::Core

/**
 * @brief Convenience logging macros using the global logger
 */
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(OpenGeoLab::Core::getLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(OpenGeoLab::Core::getLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(OpenGeoLab::Core::getLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(OpenGeoLab::Core::getLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(OpenGeoLab::Core::getLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(OpenGeoLab::Core::getLogger(), __VA_ARGS__)
