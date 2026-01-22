#pragma once

#include "app/service.hpp"
#include "util/occ_progress.hpp"

#include <algorithm>
#include <kangaroo/util/current_thread.hpp>

namespace OpenGeoLab::Util {

/// Creates a Util::ProgressCallback that reports to App::IProgressReporter.
///
/// - `base` and `span` map input progress [0,1] into [base, base+span]
/// - returns false when cancellation is requested
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
