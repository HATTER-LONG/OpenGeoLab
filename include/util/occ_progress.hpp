/**
 * @file occ_progress.hpp
 * @brief OpenCASCADE progress indicator integration
 *
 * Provides utilities to bridge between OpenCASCADE's progress reporting
 * system and the application's callback-based progress interface.
 */

#pragma once

#include "util/progress_callback.hpp"

#include <atomic>
#include <memory>
#include <string>

#include <Message_ProgressIndicator.hxx>

namespace OpenGeoLab::Util {

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
 * @brief Create an OCC progress indicator that reports via callback
 * @param callback Progress reporting callback
 * @param prefix Message prefix for human-readable status
 * @param min_delta Minimum progress change to trigger update (throttling)
 * @return OCC progress context with indicator and cancellation support
 */
[[nodiscard]] OccProgressContext
makeOccProgress(ProgressCallback callback, std::string prefix = {}, double min_delta = 0.01);

} // namespace OpenGeoLab::Util
