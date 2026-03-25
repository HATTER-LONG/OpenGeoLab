/**
 * @file request_service.hpp
 * @brief Async request-response service with requestId tracking and progress integration.
 */
#pragma once

#include <QFuture>
#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace OpenGeoLab::Python {
class EmbeddedPythonRuntime;
} // namespace OpenGeoLab::Python

namespace OpenGeoLab::Geometry {
class GeometryModule;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::App {

class ProgressTracker;

/**
 * @brief Manages async and main-thread request execution with UUID tracking.
 *
 * Each request gets a unique requestId injected into the JSON envelope.
 * Progress is reported through ProgressTracker.
 */
class RequestService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    /**
     * @brief Construct a request service bound to Python and progress services.
     * @param runtime Embedded Python runtime used for non-geometry requests.
     * @param progress_tracker Progress tracker updated during request execution.
     * @param parent Optional QObject parent.
     */
    explicit RequestService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
                            ProgressTracker& progress_tracker,
                            QObject* parent = nullptr);

    /** @brief Waits for all pending futures before destruction. */
    ~RequestService() override;

    /**
     * @brief Submit an async request dispatched to a worker thread.
     * @param request_json JSON request envelope.
     * @return Generated requestId (UUID without braces).
     */
    Q_INVOKABLE QString submitAsync(const QString& request_json);

    /**
     * @brief Execute a request synchronously on the main thread.
     *
     * Must be called from the main thread. Used for PySide6 UI operations.
     * @param request_json JSON request envelope.
     * @return Generated requestId (UUID without braces).
     */
    Q_INVOKABLE QString executeOnMainThread(const QString& request_json);

    /**
     * @brief Attach the geometry module used for OCC-backed geometry requests.
     * @param module Geometry module instance, or @c nullptr to disable geometry handling.
     */
    void setGeometryModule(Geometry::GeometryModule* module);

    /** @brief Check whether any asynchronous requests are still running. */
    [[nodiscard]] bool isBusy() const;

signals:
    /**
     * @brief Emitted when a request completes with a structured JSON response.
     * @param request_id Request identifier assigned by the service.
     * @param response_json JSON response payload returned by the request handler.
     */
    void responseReady(const QString& request_id, const QString& response_json);

    /**
     * @brief Emitted when request processing fails before a response is produced.
     * @param request_id Request identifier assigned by the service.
     * @param error_message Human-readable error description.
     */
    void errorOccurred(const QString& request_id, const QString& error_message);

    /** @brief Emitted when the busy state changes due to request scheduling or completion. */
    void busyChanged();

private:
    /** @brief Parsed request metadata prepared in a single JSON parse. */
    struct PreparedRequest {
        QString requestId;
        QString description;
        QString injectedJson;
        bool muted = false;
        QString module;
    };

    /** @brief Parse request JSON once, extract metadata, and inject requestId. */
    static PreparedRequest prepareRequest(const QString& json);

    /** @brief Parse response and emit responseReady or errorOccurred. */
    void emitResponse(const QString& request_id, const QString& response);
    QString processGeometry(const std::string& json, const QString& taskId, bool muted);

    OpenGeoLab::Python::EmbeddedPythonRuntime& m_runtime;
    ProgressTracker& m_progressTracker;
    Geometry::GeometryModule* geometryModule_ = nullptr;
    std::atomic<int> m_pendingCount{0};
    mutable std::mutex m_futuresMutex;
    std::vector<QFuture<QString>> m_pendingFutures;
};

} // namespace OpenGeoLab::App
