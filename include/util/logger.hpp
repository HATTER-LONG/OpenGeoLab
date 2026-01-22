/**
 * @file logger.hpp
 * @brief Global logger access and convenience logging macros
 */

#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace OpenGeoLab {

/**
 * @brief Get the global logger instance
 *
 * The logger is created on first use and shared across the process.
 *
 * @return Shared pointer to the global logger
 * @note Initialization is thread-safe in C++11 and later
 */
[[nodiscard]] std::shared_ptr<spdlog::logger> getLogger();

} // namespace OpenGeoLab

/**
 * @brief Convenience logging macros using the global logger
 */
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(OpenGeoLab::getLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(OpenGeoLab::getLogger(), __VA_ARGS__)
