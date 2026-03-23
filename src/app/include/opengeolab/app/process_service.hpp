/**
 * @file process_service.hpp
 * @brief Qt service bridging QML requests to the embedded Python runtime.
 */

#pragma once

#include <opengeolab/python/embedded_python_runtime.hpp>

#include <QFuture>
#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>
#include <vector>

namespace OpenGeoLab::App {

/**
 * @brief Bridges QML requests to the embedded Python runtime.
 */
class ProcessService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    /**
     * @brief Construct the bridge service with a shared embedded runtime.
     * @param runtime Embedded Python runtime used to process JSON requests.
     * @param parent Owning QObject parent.
     */
    explicit ProcessService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
                            QObject* parent = nullptr);

    /// @brief Waits for all pending async requests to finish before destruction.
    ~ProcessService() override;

    /**
     * @brief Submit a JSON request from QML to the Python runtime.
     * @param request_json JSON envelope containing separate module and action fields.
     */
    Q_INVOKABLE void submitRequest(const QString& request_json);

    /**
     * @brief Report whether any asynchronous request is still running.
     * @return True when one or more non-UI requests are pending.
     */
    [[nodiscard]] bool isBusy() const;

signals:
    /**
     * @brief Emitted when the Python runtime returns a successful JSON response.
     * @param response_json JSON response envelope returned by the runtime.
     */
    void responseReady(const QString& response_json);

    /**
     * @brief Emitted when request processing fails or returns an error envelope.
     * @param error_message Human-readable failure summary.
     */
    void errorOccurred(const QString& error_message);

    /**
     * @brief Emitted when the busy state changes.
     */
    void busyChanged();

private:
    OpenGeoLab::Python::EmbeddedPythonRuntime& m_runtime;
    std::atomic<int> m_pendingCount{0};
    mutable std::mutex m_futuresMutex;
    std::vector<QFuture<QString>> m_pendingFutures;
};

} // namespace OpenGeoLab::App
