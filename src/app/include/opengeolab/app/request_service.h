/**
 * @file request_service.h
 * @brief Async request-response service bridging QML to embedded Python runtime.
 *
 * Each request receives a unique UUID injected into the JSON envelope.
 * Async requests execute on a worker thread; main-thread requests are
 * used for PySide6 UI operations that require main-thread affinity.
 */

#pragma once

#include <QFuture>
#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>
#include <vector>

namespace OpenGeoLab::PythonEmbed {
class EmbeddedPythonRuntime;
} // namespace OpenGeoLab::PythonEmbed

namespace OpenGeoLab::App {

/**
 * @brief QML-exposed service that dispatches JSON requests through EmbeddedPythonRuntime.
 *
 * Provides two execution modes:
 * - submitAsync(): worker thread for long-running scripts.
 * - executeOnMainThread(): main thread for PySide6 UI creation.
 *
 * Emits responseReady / errorOccurred after each request completes.
 */
class RequestService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit RequestService(OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                            QObject* parent = nullptr);

    /** @brief Waits for all pending async futures before destruction. */
    ~RequestService() override;

    /**
     * @brief Submit a request dispatched to a worker thread.
     * @param request_json JSON request envelope.
     * @return Generated requestId (UUID without braces).
     */
    Q_INVOKABLE QString submitAsync(const QString& request_json);

    /**
     * @brief Execute a request synchronously on the main thread.
     *
     * Must be called from the main thread. Required for PySide6 UI operations
     * where Qt widget creation needs main-thread affinity.
     * @param request_json JSON request envelope.
     * @return Generated requestId (UUID without braces).
     */
    Q_INVOKABLE QString executeOnMainThread(const QString& request_json);

    /** @brief True when at least one async request is in flight. */
    [[nodiscard]] bool isBusy() const;

signals:
    void responseReady(const QString& request_id, const QString& response_json);
    void errorOccurred(const QString& request_id, const QString& error_message);
    void busyChanged();

private:
    struct PreparedRequest {
        QString requestId;
        QString description;
        QString injectedJson;
        bool muted = false;
    };

    static PreparedRequest prepareRequest(const QString& json);
    void emitResponse(const QString& request_id, const QString& response);

    OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& m_runtime;
    std::atomic<int> m_pendingCount{0};
    mutable std::mutex m_futuresMutex;
    std::vector<QFuture<QString>> m_pendingFutures;
};

} // namespace OpenGeoLab::App
