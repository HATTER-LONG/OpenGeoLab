/**
 * @file occ_progress.cpp
 * @brief Implementation of OpenCASCADE progress indicator adapter
 *
 * Bridges OpenCASCADE's Message_ProgressIndicator to our callback-based
 * progress system, with support for cancellation and throttled updates.
 */

#include "util/occ_progress.hpp"

#include <algorithm>
#include <atomic>
#include <sstream>
#include <vector>

#include <Message_ProgressScope.hxx>

namespace OpenGeoLab::Util {

namespace {

/**
 * @brief Join scope names into a hierarchical path string
 * @param scope Current progress scope
 * @return Path string like "Reading / Parsing / Vertices"
 */
std::string joinScopePath(const Message_ProgressScope& scope) {
    std::vector<std::string> parts;
    for(const Message_ProgressScope* current = &scope; current != nullptr;
        current = current->Parent()) {
        const char* name = current->Name();
        if(name != nullptr && name[0] != '\0') {
            parts.emplace_back(name);
        }
    }
    if(parts.empty()) {
        return {};
    }
    std::reverse(parts.begin(), parts.end());

    std::ostringstream os;
    for(size_t i = 0; i < parts.size(); ++i) {
        if(i != 0) {
            os << " / ";
        }
        os << parts[i];
    }
    return os.str();
}

/**
 * @brief OpenCASCADE progress indicator that delegates to a callback
 *
 * Implements throttling to avoid excessive callback invocations and
 * supports cooperative cancellation via atomic flag.
 */
class CallbackProgressIndicator final : public Message_ProgressIndicator {
public:
    /**
     * @brief Construct progress indicator
     * @param callback Progress callback function
     * @param cancelled Shared cancellation flag
     * @param prefix Message prefix for progress reports
     * @param min_delta Minimum progress delta before reporting
     */
    CallbackProgressIndicator(ProgressCallback callback,
                              std::shared_ptr<std::atomic_bool> cancelled,
                              std::string prefix,
                              double min_delta)
        : m_callback(std::move(callback)), m_cancelled(std::move(cancelled)),
          m_prefix(std::move(prefix)), m_minDelta(min_delta) {}

protected:
    /**
     * @brief Check for user-requested cancellation
     * @return Standard_True if cancelled, Standard_False otherwise
     */
    Standard_Boolean UserBreak() override {
        return (m_cancelled && m_cancelled->load(std::memory_order_relaxed)) ? Standard_True
                                                                             : Standard_False;
    }

    /**
     * @brief Called by OCC when progress updates
     * @param scope Current progress scope with hierarchy info
     * @param is_force If true, bypass throttling and report immediately
     */
    void Show(const Message_ProgressScope& scope, const Standard_Boolean is_force) override {
        if(!m_callback) {
            return;
        }

        const double position = static_cast<double>(GetPosition());

        // Throttle updates unless forced
        if(!is_force && m_lastPosition >= 0.0 && (position - m_lastPosition) < m_minDelta) {
            return;
        }
        m_lastPosition = position;

        // Build human-readable message from prefix and scope path
        std::string message;
        const std::string scope_path = joinScopePath(scope);
        if(!m_prefix.empty() && !scope_path.empty()) {
            message = m_prefix + ": " + scope_path;
        } else if(!m_prefix.empty()) {
            message = m_prefix;
        } else if(!scope_path.empty()) {
            message = scope_path;
        } else {
            message = "Working...";
        }

        // Invoke callback; false return means cancellation requested
        const bool keep_going = m_callback(position, message);
        if(!keep_going && m_cancelled) {
            m_cancelled->store(true, std::memory_order_relaxed);
        }
    }

private:
    ProgressCallback m_callback;
    std::shared_ptr<std::atomic_bool> m_cancelled;
    std::string m_prefix;
    double m_minDelta{0.01};
    double m_lastPosition{-1.0};
};

} // namespace

bool OccProgressContext::isCancelled() const {
    return m_cancelled && m_cancelled->load(std::memory_order_relaxed);
}

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

OccProgressContext
makeOccProgress(ProgressCallback callback, std::string prefix, double min_delta) {
    OccProgressContext ctx;
    if(!callback) {
        return ctx;
    }

    ctx.m_cancelled = std::make_shared<std::atomic_bool>(false);
    ctx.m_indicator = new CallbackProgressIndicator(std::move(callback), ctx.m_cancelled,
                                                    std::move(prefix), min_delta);
    return ctx;
}

} // namespace OpenGeoLab::Util
