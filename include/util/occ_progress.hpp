#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <Message_ProgressIndicator.hxx>

namespace OpenGeoLab::Util {

using ProgressCallback = std::function<bool(double progress, const std::string& message)>;

struct OccProgressContext {
    Handle(Message_ProgressIndicator) m_indicator;
    std::shared_ptr<std::atomic_bool> m_cancelled;

    bool isCancelled() const;
};

/// Wraps a callback to map progress from [0,1] into [base, base+span].
ProgressCallback makeScaledProgressCallback(ProgressCallback callback, double base, double span);

/// Create OCCT progress indicator that reports via callback and supports cancellation.
///
/// - `prefix` is prepended to scope path (if any) to form a human readable message.
/// - `min_delta` throttles updates (e.g. 0.01 = 1%).
OccProgressContext
makeOccProgress(ProgressCallback callback, std::string prefix = {}, double min_delta = 0.01);

} // namespace OpenGeoLab::Util
