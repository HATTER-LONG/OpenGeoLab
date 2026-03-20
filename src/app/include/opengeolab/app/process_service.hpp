/**
 * @file process_service.hpp
 * @brief Qt service bridging QML requests to the embedded Python runtime.
 */

#pragma once

#include <opengeolab/python/embedded_python_runtime.hpp>

#include <QObject>
#include <QString>
#include <atomic>

namespace OpenGeoLab::App {

/**
 * @brief Bridges QML requests to the embedded Python runtime.
 */
class ProcessService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    /**
     * @brief Constructs the process service.
     * @param runtime Embedded Python runtime used to process request payloads.
     * @param parent Owning Qt parent object.
     */
    explicit ProcessService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
                            QObject* parent = nullptr);

    /**
     * @brief Submits a JSON request for synchronous UI handling or asynchronous processing.
     * @param requestJson Serialized protocol request from QML.
     */
    Q_INVOKABLE void submitRequest(const QString& requestJson);

    /**
     * @brief Reports whether the service currently has one or more pending requests.
     * @return True when asynchronous request processing is still in flight.
     */
    [[nodiscard]] bool isBusy() const;

signals:
    /**
     * @brief Emits the successful JSON response for a completed request.
     * @param requestId Request identifier extracted from the submitted payload.
     * @param responseJson Serialized protocol response produced by the runtime.
     */
    void responseReady(const QString& requestId, const QString& responseJson);

    /**
     * @brief Emits the error summary for a failed request.
     * @param requestId Request identifier extracted from the submitted payload.
     * @param errorMessage Error summary propagated to the UI layer.
     */
    void errorOccurred(const QString& requestId, const QString& errorMessage);

    /**
     * @brief Emits when the busy state changes.
     */
    void busyChanged();

private:
    OpenGeoLab::Python::EmbeddedPythonRuntime& m_runtime;
    std::atomic<int> m_pendingCount{0};
    // Extension point: IRequestRecorder* m_recorder = nullptr;
};

} // namespace OpenGeoLab::App
