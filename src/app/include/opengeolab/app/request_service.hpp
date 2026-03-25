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

    void setGeometryModule(Geometry::GeometryModule* module);

    [[nodiscard]] bool isBusy() const;

signals:
    void responseReady(const QString& request_id, const QString& response_json);
    void errorOccurred(const QString& request_id, const QString& error_message);
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
