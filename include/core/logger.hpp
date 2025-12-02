/**
 * @file logger.hpp
 * @brief Global logger instance and convenience macros for the OpenGeoLab project
 *
 * This file provides a centralized logging system using spdlog for consistent
 * logging across the entire OpenGeoLab codebase. It includes:
 * - A singleton logger instance accessible throughout the project
 * - Convenient logging macros for all severity levels (TRACE through CRITICAL)
 * - Integration with the Kangaroo library's logger factory
 *
 * @example
 * @code
 * LOG_INFO("Application started successfully");
 * LOG_ERROR("Failed to load file: {}", filename);
 * LOG_DEBUG("Variable value: x={}, y={}", x, y);
 * @endcode
 */

#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace OpenGeoLab {

/**
 * @brief Get the global logger instance for the OpenGeoLab project
 *
 * Returns a singleton logger instance that is initialized with trace level logging.
 * The logger is created on first access using Meyer's singleton pattern,
 * which ensures:
 * - Thread-safe initialization (C++11 and later)
 * - No static initialization order issues
 * - Automatic cleanup on program exit
 * - Only one instance exists throughout the program lifetime
 *
 * @return Shared pointer to the global logger instance
 *
 * @note This function is thread-safe in C++11 and later
 */
std::shared_ptr<spdlog::logger> getLogger();

} // namespace OpenGeoLab

// ============================================================================
// Convenience logging macros using the global OpenGeoLab logger
// ============================================================================

/**
 * @brief Log a trace message (most verbose level)
 *
 * Use for very detailed diagnostic information, typically only enabled
 * during development or when debugging specific issues.
 */
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(OpenGeoLab::getLogger(), __VA_ARGS__)

/**
 * @brief Log a debug message
 *
 * Use for diagnostic information useful during development and debugging.
 */
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(OpenGeoLab::getLogger(), __VA_ARGS__)

/**
 * @brief Log an informational message
 *
 * Use for general informational messages about program execution,
 * such as startup, shutdown, or significant state changes.
 */
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(OpenGeoLab::getLogger(), __VA_ARGS__)

/**
 * @brief Log a warning message
 *
 * Use for potentially problematic situations that don't prevent
 * the program from continuing but may require attention.
 */
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(OpenGeoLab::getLogger(), __VA_ARGS__)

/**
 * @brief Log an error message
 *
 * Use for error conditions that prevent a specific operation from
 * completing successfully but don't terminate the program.
 */
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(OpenGeoLab::getLogger(), __VA_ARGS__)

/**
 * @brief Log a critical message (highest severity)
 *
 * Use for severe error conditions that may lead to program termination
 * or major functionality failure.
 */
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(OpenGeoLab::getLogger(), __VA_ARGS__)

