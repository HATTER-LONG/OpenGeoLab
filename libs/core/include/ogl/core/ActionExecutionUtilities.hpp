/**
 * @file ActionExecutionUtilities.hpp
 * @brief Shared helpers for action progress reporting, early cancellation, and Python parity.
 */

#pragma once

#include <ogl/core/IService.hpp>

#include <functional>
#include <sstream>
#include <string_view>
#include <utility>

namespace OGL::Core {

/**
 * @brief Report action progress if a callback is provided.
 * @param progressCallback Optional callback supplied by the caller.
 * @param progress Progress value in the range [0.0, 1.0].
 * @param message Human-readable stage description.
 * @return False when the callback requests cancellation.
 */
inline auto reportProgress(const ProgressCallback& progressCallback,
                           double progress,
                           std::string_view message) -> bool {
    return !progressCallback || progressCallback(progress, std::string{message});
}

/**
 * @brief Build a standard cancellation response for an interrupted action.
 * @param request Original service request.
 * @param message User-facing cancellation message.
 * @return Failure-shaped response with an empty payload object.
 */
inline auto buildCancellationResponse(const ServiceRequest& request, std::string_view message)
    -> ServiceResponse {
    return {.success = false,
            .module = request.module,
            .action = request.action,
            .message = std::string{message},
            .payload = nlohmann::json::object()};
}

/**
 * @brief Build a standard failure response for action-layer validation or execution errors.
 * @param request Original service request.
 * @param message User-facing failure message.
 * @return Failure-shaped response with an empty payload object.
 */
inline auto buildFailureResponse(const ServiceRequest& request, std::string_view message)
    -> ServiceResponse {
    return {.success = false,
            .module = request.module,
            .action = request.action,
            .message = std::string{message},
            .payload = nlohmann::json::object()};
}

/**
 * @brief Build a Python bridge snippet that reproduces the current request envelope.
 * @param request Original service request or a normalized request variant.
 * @return Python code suitable for bridge-based replay.
 */
inline auto buildEquivalentPythonSnippet(const ServiceRequest& request) -> std::string {
    const auto requestJson = request.toJson().dump(2);
    const auto pythonStringLiteral = nlohmann::json(requestJson).dump();

    std::ostringstream script;
    script << "import json\n";
    script << "import opengeolab\n\n";
    script << "bridge = opengeolab.OpenGeoLabPythonBridge()\n";
    script << "request = json.loads(" << pythonStringLiteral << ")\n";
    script << "result = bridge.process(request)\n";
    script << "print(result)";
    return script.str();
}

/**
 * @brief Run one progress-aware execution stage and capture early cancellation.
 * @tparam StepFn Callable executed when the progress callback allows the stage to continue.
 * @param request Original service request.
 * @param progressCallback Optional progress callback.
 * @param progress Progress value associated with this stage.
 * @param progressMessage Human-readable stage description.
 * @param cancellationMessage Failure message stored when the callback requests cancellation.
 * @param step Callable that performs the stage work.
 * @param earlyResponse Output parameter populated on cancellation.
 * @return True when the stage ran, false when the callback requested cancellation.
 */
template <class StepFn>
inline auto runProgressStage(const ServiceRequest& request,
                             const ProgressCallback& progressCallback,
                             double progress,
                             std::string_view progressMessage,
                             std::string_view cancellationMessage,
                             StepFn&& step,
                             ServiceResponse& earlyResponse) -> bool {
    if(!reportProgress(progressCallback, progress, progressMessage)) {
        earlyResponse = buildCancellationResponse(request, cancellationMessage);
        return false;
    }

    static_cast<void>(std::invoke(std::forward<StepFn>(step)));
    return true;
}

} // namespace OGL::Core
