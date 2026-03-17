#include "OpenGeoLabRequestExecutor.hpp"

#include <QFutureWatcher>
#include <QMetaObject>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>

namespace OGL::App {

namespace {

constexpr const char* kBusyMessage = "Another async service request is active.";

} // namespace

OpenGeoLabRequestExecutor::OpenGeoLabRequestExecutor(
    std::unique_ptr<OGL::Command::CommandRecorder> commandRecorder, ExecuteOverride executeOverride)
    : m_commandRecorder(std::move(commandRecorder)), m_executeOverride(std::move(executeOverride)) {
}

auto OpenGeoLabRequestExecutor::buildFailureResponse(const std::string& module,
                                                     const std::string& action,
                                                     const std::string& message,
                                                     const std::string& reason) -> nlohmann::json {
    nlohmann::json payload = nlohmann::json::object();
    if(!reason.empty()) {
        payload["reason"] = reason;
    }
    return {{"success", false},
            {"module", module},
            {"action", action},
            {"message", message},
            {"payload", std::move(payload)}};
}

auto OpenGeoLabRequestExecutor::enrichRequest(OGL::Command::CommandRequest request,
                                              const QString& source) const
    -> OGL::Command::CommandRequest {
    if(!source.trimmed().isEmpty() && !request.param.contains("source")) {
        request.param["source"] = source.toStdString();
    }
    return request;
}

auto OpenGeoLabRequestExecutor::currentRecorderState() const -> RecorderStateEvent {
    std::scoped_lock lock(m_commandExecutionMutex);
    return {.recordedCommandCount = m_commandRecorder->recordedCount(),
            .recordedCommands = QString::fromStdString(m_commandRecorder->historyJson().dump(2))};
}

auto OpenGeoLabRequestExecutor::executeRequest(const OGL::Command::CommandRequest& request,
                                               int requestId,
                                               const EventSinks& eventSinks) -> nlohmann::json {
    const auto progressCallback = [&eventSinks, requestId](double progress,
                                                           const std::string& message) {
        if(eventSinks.onProgress) {
            eventSinks.onProgress({.requestId = requestId,
                                   .active = true,
                                   .progress = std::clamp(progress, 0.0, 1.0),
                                   .message = QString::fromStdString(message),
                                   .state = QStringLiteral("running")});
        }
        return true;
    };

    std::scoped_lock lock(m_commandExecutionMutex);
    if(m_executeOverride) {
        return m_executeOverride(*m_commandRecorder, request, progressCallback);
    }
    return m_commandRecorder->execute(request, progressCallback).toJson();
}

auto OpenGeoLabRequestExecutor::executeSync(OGL::Command::CommandRequest request,
                                            const QString& source,
                                            const EventSinks& eventSinks,
                                            BusyPolicy busyPolicy) -> nlohmann::json {
    request = enrichRequest(std::move(request), source);
    if(busyPolicy == BusyPolicy::FailIfAsyncActive && m_asyncServiceRequestActive.load()) {
        const auto busyResponse =
            buildFailureResponse(request.module, request.action, kBusyMessage, "request-busy");
        if(eventSinks.onRequestFinished) {
            eventSinks.onRequestFinished({.requestId = 0, .response = busyResponse});
        }
        return busyResponse;
    }

    const auto response = executeRequest(request, 0, eventSinks);
    if(eventSinks.onRequestFinished) {
        eventSinks.onRequestFinished({.requestId = 0, .response = response});
    }
    if(eventSinks.onRecorderStateChanged) {
        eventSinks.onRecorderStateChanged(currentRecorderState());
    }
    return response;
}

void OpenGeoLabRequestExecutor::dispatchToContext(QObject* context,
                                                  std::function<void()> fn,
                                                  Qt::ConnectionType connectionType) {
    if(context == nullptr || QThread::currentThread() == context->thread()) {
        fn();
        return;
    }

    QMetaObject::invokeMethod(context, [fn = std::move(fn)]() mutable { fn(); }, connectionType);
}

auto OpenGeoLabRequestExecutor::submitAsync(QObject* callbackContext,
                                            OGL::Command::CommandRequest request,
                                            const QString& source,
                                            const EventSinks& eventSinks) -> int {
    if(m_asyncServiceRequestActive.exchange(true)) {
        return -1;
    }

    const int requestId = m_nextAsyncRequestId++;
    auto* watcher = new QFutureWatcher<AsyncExecutionResult>(callbackContext);
    const auto enrichedRequest = enrichRequest(std::move(request), source);

    QObject::connect(watcher, &QFutureWatcher<AsyncExecutionResult>::finished, watcher,
                     [this, watcher, eventSinks]() {
                         const AsyncExecutionResult result = watcher->result();
                         m_asyncServiceRequestActive = false;
                         if(eventSinks.onRequestFinished) {
                             eventSinks.onRequestFinished(
                                 {.requestId = result.requestId, .response = result.response});
                         }
                         if(eventSinks.onRecorderStateChanged) {
                             eventSinks.onRecorderStateChanged(result.recorderState);
                         }
                         watcher->deleteLater();
                     });

    watcher->setFuture(QtConcurrent::run([this, callbackContext, eventSinks, requestId,
                                          request = enrichedRequest]() mutable {
        EventSinks workerEventSinks = eventSinks;
        workerEventSinks.onProgress =
            [callbackContext, onProgress = eventSinks.onProgress](const ProgressEvent& event) {
                if(!onProgress) {
                    return;
                }
                dispatchToContext(
                    callbackContext, [onProgress, event]() { onProgress(event); },
                    Qt::BlockingQueuedConnection);
            };

        const auto response = executeRequest(request, requestId, workerEventSinks);
        return AsyncExecutionResult{
            .requestId = requestId, .response = response, .recorderState = currentRecorderState()};
    }));

    return requestId;
}

auto OpenGeoLabRequestExecutor::replayRecordedCommands(const EventSinks& eventSinks)
    -> nlohmann::json {
    nlohmann::json response;
    {
        std::scoped_lock lock(m_commandExecutionMutex);
        const auto replayReport = m_commandRecorder->replayAll();
        const bool success = replayReport.replayedCount == replayReport.successCount;
        response = {{"success", success},
                    {"module", "command"},
                    {"action", "replayRecordedCommands"},
                    {"message", success ? "Recorded commands replayed successfully."
                                        : "Recorded command replay finished with failures."},
                    {"payload",
                     {{"summary", QStringLiteral("Replayed %1 command(s); %2 succeeded.")
                                      .arg(replayReport.replayedCount)
                                      .arg(replayReport.successCount)
                                      .toStdString()},
                      {"replayReport", replayReport.toJson()},
                      {"exportedPython", m_commandRecorder->exportedPythonScript()},
                      {"recordedCommands", m_commandRecorder->historyJson()},
                      {"recordedCommandCount", m_commandRecorder->recordedCount()}}}};
    }

    if(eventSinks.onRequestFinished) {
        eventSinks.onRequestFinished({.requestId = 0, .response = response});
    }
    if(eventSinks.onRecorderStateChanged) {
        eventSinks.onRecorderStateChanged(currentRecorderState());
    }
    return response;
}

auto OpenGeoLabRequestExecutor::clearRecordedCommands(const EventSinks& eventSinks)
    -> nlohmann::json {
    nlohmann::json response;
    {
        std::scoped_lock lock(m_commandExecutionMutex);
        m_commandRecorder->clear();
        response = {{"success", true},
                    {"module", "command"},
                    {"action", "clearRecordedCommands"},
                    {"message", "Recorded commands cleared."},
                    {"payload",
                     {{"summary", "Command history cleared."},
                      {"exportedPython", m_commandRecorder->exportedPythonScript()},
                      {"recordedCommands", m_commandRecorder->historyJson()},
                      {"recordedCommandCount", m_commandRecorder->recordedCount()}}}};
    }

    if(eventSinks.onRequestFinished) {
        eventSinks.onRequestFinished({.requestId = 0, .response = response});
    }
    if(eventSinks.onRecorderStateChanged) {
        eventSinks.onRecorderStateChanged(currentRecorderState());
    }
    return response;
}

auto OpenGeoLabRequestExecutor::recorderSnapshot() const -> RecorderSnapshot {
    std::scoped_lock lock(m_commandExecutionMutex);
    return {.recordedCommandCount = m_commandRecorder->recordedCount(),
            .recordedCommands = m_commandRecorder->historyJson(),
            .exportedPython = QString::fromStdString(m_commandRecorder->exportedPythonScript())};
}

auto OpenGeoLabRequestExecutor::isAsyncRequestActive() const -> bool {
    return m_asyncServiceRequestActive.load();
}

} // namespace OGL::App
