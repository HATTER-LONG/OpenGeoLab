/**
 * @file request_service.h
 * @brief Async request-response service bridging QML to embedded Python runtime.
 *
 * Async requests execute on a worker thread; main-thread requests are
 * used for PySide6 UI operations that require main-thread affinity.
 * Progress updates from C++ actions are forwarded to QML via progressUpdated.
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
 * Emits progressUpdated during long-running operations.
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
     */
    Q_INVOKABLE void submitAsync(const QString& request_json);

    /**
     * @brief Execute a request synchronously on the main thread.
     *
     * Must be called from the main thread. Required for PySide6 UI operations
     * where Qt widget creation needs main-thread affinity.
     * @param request_json JSON request envelope.
     */
    Q_INVOKABLE void executeOnMainThread(const QString& request_json);

    /** @brief True when at least one async request is in flight. */
    [[nodiscard]] bool isBusy() const;

signals:
    void responseReady(const QString& response_json, bool muted);
    void errorOccurred(const QString& error_message, bool muted);
    void busyChanged();

    /**
     * @brief Emitted when a request is dispatched.
     * @param description Short label, e.g. "io.read_brep".
     * @param request_json Original JSON envelope before injection.
     * @param muted True if the request should be hidden from progress UI.
     */
    void requestSent(const QString& description, const QString& request_json, bool muted);

    /**
     * @brief Emitted when a long-running action reports progress.
     * @param progress Value in [0, 1] range.
     * @param message Human-readable status message.
     */
    void progressUpdated(double progress, const QString& message);

private:
    struct PreparedRequest {
        QString description;
        QString processJson; ///< JSON sent to the Python runtime (original, no injection).
        bool muted = false;
    };

    static PreparedRequest prepareRequest(const QString& json);
    void emitResponse(const QString& response, bool muted);

    OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& m_runtime;
    std::atomic<int> m_pendingCount{0};
    mutable std::mutex m_futuresMutex;
    std::vector<QFuture<QString>> m_pendingFutures;
};

} // namespace OpenGeoLab::App
