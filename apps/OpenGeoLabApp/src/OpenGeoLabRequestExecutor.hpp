/**
 * @file OpenGeoLabRequestExecutor.hpp
 * @brief Executes synchronous and asynchronous command requests while keeping recorder access
 * serialized.
 */

#pragma once

#include "OpenGeoLabControllerEvents.hpp"

#include <QObject>

#include <ogl/command/CommandService.hpp>
#include <ogl/core/IService.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

namespace OGL::App {

class OpenGeoLabRequestExecutor {
public:
    enum class BusyPolicy { WaitForAvailability, FailIfAsyncActive };

    struct EventSinks {
        std::function<void(const ProgressEvent&)> onProgress;
        std::function<void(const RequestFinishedEvent&)> onRequestFinished;
        std::function<void(const RecorderStateEvent&)> onRecorderStateChanged;
    };

    struct RecorderSnapshot {
        int recordedCommandCount{0};
        nlohmann::json recordedCommands = nlohmann::json::array();
        QString exportedPython;
    };

    using ExecuteOverride = std::function<nlohmann::json(OGL::Command::CommandRecorder&,
                                                         const OGL::Command::CommandRequest&,
                                                         const OGL::Core::ProgressCallback&)>;

    explicit OpenGeoLabRequestExecutor(
        std::unique_ptr<OGL::Command::CommandRecorder> commandRecorder =
            std::make_unique<OGL::Command::CommandRecorder>(),
        ExecuteOverride executeOverride = {});

    auto executeSync(OGL::Command::CommandRequest request,
                     const QString& source,
                     const EventSinks& eventSinks,
                     BusyPolicy busyPolicy = BusyPolicy::WaitForAvailability) -> nlohmann::json;
    auto submitAsync(QObject* callbackContext,
                     OGL::Command::CommandRequest request,
                     const QString& source,
                     const EventSinks& eventSinks) -> int;
    auto replayRecordedCommands(const EventSinks& eventSinks) -> nlohmann::json;
    auto clearRecordedCommands(const EventSinks& eventSinks) -> nlohmann::json;

    [[nodiscard]] auto recorderSnapshot() const -> RecorderSnapshot;
    [[nodiscard]] auto isAsyncRequestActive() const -> bool;

    [[nodiscard]] static auto buildFailureResponse(const std::string& module,
                                                   const std::string& action,
                                                   const std::string& message,
                                                   const std::string& reason = std::string{})
        -> nlohmann::json;

private:
    struct AsyncExecutionResult {
        int requestId{0};
        nlohmann::json response = nlohmann::json::object();
        RecorderStateEvent recorderState;
    };

    [[nodiscard]] auto enrichRequest(OGL::Command::CommandRequest request,
                                     const QString& source) const -> OGL::Command::CommandRequest;
    [[nodiscard]] auto currentRecorderState() const -> RecorderStateEvent;
    auto executeRequest(const OGL::Command::CommandRequest& request,
                        int requestId,
                        const EventSinks& eventSinks) -> nlohmann::json;
    static void dispatchToContext(QObject* context,
                                  std::function<void()> fn,
                                  Qt::ConnectionType connectionType);

    std::unique_ptr<OGL::Command::CommandRecorder> m_commandRecorder;
    ExecuteOverride m_executeOverride;
    mutable std::mutex m_commandExecutionMutex;
    std::atomic<bool> m_asyncServiceRequestActive{false};
    std::atomic<int> m_nextAsyncRequestId{1};
};

} // namespace OGL::App
