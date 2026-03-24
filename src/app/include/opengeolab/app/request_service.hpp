/// @file request_service.hpp
/// @brief Async request-response service with requestId tracking and progress integration.
#pragma once

#include <QFuture>
#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>
#include <vector>

namespace OpenGeoLab::Python {
class EmbeddedPythonRuntime;
} // namespace OpenGeoLab::Python

namespace OpenGeoLab::App {

class ProgressTracker;

/// @brief Manages async and main-thread request execution with UUID tracking.
///
/// Each request gets a unique requestId injected into the JSON envelope.
/// Progress is reported through ProgressTracker.
class RequestService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit RequestService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
                            ProgressTracker& progress_tracker,
                            QObject* parent = nullptr);

    /// @brief Waits for all pending futures before destruction.
    ~RequestService() override;

    /// @brief Submit an async request dispatched to a worker thread.
    /// @param request_json JSON request envelope.
    /// @return Generated requestId (UUID without braces).
    Q_INVOKABLE QString submitAsync(const QString& request_json);

    /// @brief Execute a request synchronously on the main thread.
    /// Must be called from the main thread. Used for PySide6 UI operations.
    /// @param request_json JSON request envelope.
    /// @return Generated requestId (UUID without braces).
    Q_INVOKABLE QString executeOnMainThread(const QString& request_json);

    [[nodiscard]] bool isBusy() const;

signals:
    void responseReady(const QString& request_id, const QString& response_json);
    void errorOccurred(const QString& request_id, const QString& error_message);
    void busyChanged();

private:
    /// @brief Inject requestId field into JSON, return modified JSON string.
    static QString injectRequestId(const QString& json, const QString& request_id);

    /// @brief Extract "module.action" description from JSON for ProgressTracker.
    static QString extractDescription(const QString& json);

    /// @brief Parse response and emit responseReady or errorOccurred.
    void emitResponse(const QString& request_id, const QString& response);

    OpenGeoLab::Python::EmbeddedPythonRuntime& m_runtime;
    ProgressTracker& m_progressTracker;
    std::atomic<int> m_pendingCount{0};
    mutable std::mutex m_futuresMutex;
    std::vector<QFuture<QString>> m_pendingFutures;
};

} // namespace OpenGeoLab::App
