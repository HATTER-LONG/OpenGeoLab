/**
 * @file progress_bridge.hpp
 * @brief Bridge between service progress reporter and utility callbacks
 *
 * Provides conversion utilities to use IProgressReporter with the
 * ProgressCallback interface used by readers and other utilities.
 */

#pragma once

#include "app/service.hpp"
#include "util/progress_callback.hpp"

#include <algorithm>
#include <kangaroo/util/current_thread.hpp>

namespace OpenGeoLab::Util {

/**
 * @brief Create a ProgressCallback from an IProgressReporter
 * @param reporter Progress reporter to wrap
 * @param base Starting progress value for scaling
 * @param span Progress range for scaling
 * @return ProgressCallback that delegates to the reporter
 *
 * The returned callback maps input progress [0,1] to [base, base+span]
 * and returns false when cancellation is requested.
 */
[[nodiscard]] inline ProgressCallback makeProgressCallback(
    const App::IProgressReporterPtr& reporter, double base = 0.0, double span = 1.0) {
    if(!reporter) {
        return {};
    }

    return [reporter, base, span](double progress, const std::string& message) -> bool {
        if(reporter->isCancelled()) {
            return false;
        }
        const double clamped = std::clamp(progress, 0.0, 1.0);
        reporter->reportProgress(base + span * clamped, message);
        return !reporter->isCancelled();
    };
}

} // namespace OpenGeoLab::Util
