/**
 * @file OpenGeoLabFeedbackCoordinator.hpp
 * @brief Maps execution and automation events into controller-visible state deltas.
 */

#pragma once

#include "OpenGeoLabControllerEvents.hpp"

namespace OGL::App {

class OpenGeoLabFeedbackCoordinator {
public:
    [[nodiscard]] auto onRequestStarted(const RequestStartedEvent& event) const
        -> ControllerStateDelta;
    [[nodiscard]] auto onProgress(const ProgressEvent& event) const -> ControllerStateDelta;
    [[nodiscard]] auto onRequestFinished(const RequestFinishedEvent& event) const
        -> ControllerStateDelta;
    [[nodiscard]] auto onPythonOutput(const PythonOutputEvent& event) const -> ControllerStateDelta;
    [[nodiscard]] auto onRecorderStateChanged(const RecorderStateEvent& event) const
        -> ControllerStateDelta;
    [[nodiscard]] auto onUiNotice(const UiNotice& notice) const -> ControllerStateDelta;

    [[nodiscard]] static auto buildPublicResponse(const nlohmann::json& response) -> nlohmann::json;
};

} // namespace OGL::App
