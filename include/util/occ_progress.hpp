/**
 * @file occ_progress.hpp
 * @brief OpenCASCADE progress indicator integration
 *
 * Provides utilities to bridge between OpenCASCADE's progress reporting
 * system and the application's callback-based progress interface.
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <Message_ProgressIndicator.hxx>

namespace OpenGeoLab::Util {

/**
 * @brief Progress callback function type
 * @param progress Progress value in [0, 1] range
 * @param message Human-readable status message
 * @return false to request cancellation, true to continue
 */
using ProgressCallback = std::function<bool(double progress, const std::string& message)>;

inline const ProgressCallback NO_PROGRESS_CALLBACK = [](double, const std::string&) {
    return true;
};

/**
 * @brief Context for OCC progress indicator with cancellation support
 */
struct OccProgressContext {
    Handle(Message_ProgressIndicator) m_indicator; ///< OCC progress indicator handle
    std::shared_ptr<std::atomic_bool> m_cancelled; ///< Shared cancellation flag

    /**
     * @brief Check if operation was cancelled
     * @return true if cancellation was requested
     */
    bool isCancelled() const;
};

/**
 * @brief Create a scaled progress callback that maps to a sub-range
 * @param callback Original callback to wrap
 * @param base Starting progress value for the sub-range
 * @param span Size of the sub-range
 * @return Callback that maps [0,1] to [base, base+span]
 */
[[nodiscard]] ProgressCallback
makeScaledProgressCallback(ProgressCallback callback, double base, double span);

/**
 * @brief Create an OCC progress indicator that reports via callback
 * @param callback Progress reporting callback
 * @param prefix Message prefix for human-readable status
 * @param min_delta Minimum progress change to trigger update (throttling)
 * @return OCC progress context with indicator and cancellation support
 */
[[nodiscard]] OccProgressContext
makeOccProgress(ProgressCallback callback, std::string prefix = {}, double min_delta = 0.01);

} // namespace OpenGeoLab::Util
