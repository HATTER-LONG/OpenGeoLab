#pragma once

#include <functional>
#include <kangaroo/util/current_thread.hpp>

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
 * @brief Create a scaled progress callback that maps to a sub-range
 * @param callback Original callback to wrap
 * @param base Starting progress value for the sub-range
 * @param span Size of the sub-range
 * @return Callback that maps [0,1] to [base, base+span]
 */
[[nodiscard]] ProgressCallback
makeScaledProgressCallback(ProgressCallback callback, double base, double span);

} // namespace OpenGeoLab::Util