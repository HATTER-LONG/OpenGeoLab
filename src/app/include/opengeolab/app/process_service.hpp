#pragma once

#include <opengeolab/python/embedded_python_runtime.hpp>

#include <QObject>
#include <QString>
#include <atomic>

namespace OpenGeoLab::App {

class ProcessService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit ProcessService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
                            QObject* parent = nullptr);

    Q_INVOKABLE void submitRequest(const QString& requestJson);
    [[nodiscard]] bool isBusy() const;

signals:
    void responseReady(const QString& requestId, const QString& responseJson);
    void errorOccurred(const QString& requestId, const QString& errorMessage);
    void busyChanged();

private:
    OpenGeoLab::Python::EmbeddedPythonRuntime& m_runtime;
    std::atomic<int> m_pendingCount{0};
    // Extension point: IRequestRecorder* m_recorder = nullptr;
};

} // namespace OpenGeoLab::App
