/**
 * @file process_callback.cpp
 * @brief Implementation of progress callback utilities
 */

#include "opengeolab/core/progress_callback.hpp"
#include <algorithm>

namespace OpenGeoLab::Core {
ProgressCallback makeScaledProgressCallback(ProgressCallback callback, double base, double span) {
    if(!callback) {
        return {};
    }
    return [callback = std::move(callback), base, span](double progress,
                                                        const std::string& message) -> bool {
        const double clamped = std::clamp(progress, 0.0, 1.0);
        return callback(base + span * clamped, message);
    };
}
} // namespace OpenGeoLab::Core