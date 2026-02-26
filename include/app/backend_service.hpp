/**
 * @file backend_service.hpp
 * @brief QML-exposed backend service for asynchronous module operations
 *
 * ServiceWorker is defined in service_worker.hpp to allow direct use of
 * nlohmann::json without Qt MOC compatibility issues.
 */

#pragma once

#include <QObject>
#include <QPointer>
#include <QThread>
#include <QtQml/qqml.h>
#include <atomic>

namespace OpenGeoLab::App {
class ServiceWorker;

/**
 * @brief QML singleton service for executing backend operations asynchronously
 *
 * Provides progress reporting, cancellation support, and error handling.
 * Operations run in a separate worker thread to avoid blocking the UI.
 */
class BackendService final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    explicit BackendService(QObject* parent = nullptr);
    ~BackendService() override;

    [[nodiscard]] bool busy() const;
    [[nodiscard]] double progress() const;
    [[nodiscard]] QString message() const;

    /**
     * @brief Start an asynchronous operation
     * @param module_name Registered service module identifier
     * @param params JSON-encoded parameters for the operation
     */
    Q_INVOKABLE void request(const QString& module_name, const QString& params);

    /// Requests cancellation of the current operation
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void messageChanged();

    void operationStarted(const QString& module_name, const QString& action_name);

    void operationProgress(const QString& module_name,
                           const QString& action_name,
                           double progress,
                           const QString& message);

    void operationFinished(const QString& module_name,
                           const QString& action_name,
                           const QString& result);
    void
    operationFailed(const QString& module_name, const QString& action_name, const QString& error);

private slots:
    void onWorkerProgress(double progress, const QString& message);
    void onWorkerFinished(const QString& module_name, const QString& result);
    void onWorkerError(const QString& module_name, const QString& error);
    void onWorkerThreadFinished();

private:
    struct RequestContext {
        QString m_moduleName;
        QString m_actionName;
        bool m_silent{false};

        void reset() {
            m_moduleName.clear();
            m_actionName.clear();
            m_silent = false;
        }
    };

    struct DeferredRequest {
        bool m_pending{false};
        QString m_moduleName;
        QString m_params;

        void reset() {
            m_pending = false;
            m_moduleName.clear();
            m_params.clear();
        }
    };

    void setMessage(const QString& message);
    void setProgressInternal(double progress);
    void setBusyInternal(bool busy);
    void scheduleDeferredRequestIfNeeded();
    void processDeferredRequest();
    void cleanupWorker();

private:
    bool m_processingRequest{false};
    bool m_busy{false};
    double m_progress{0.0};

    QString m_message;
    RequestContext m_currentRequest;
    DeferredRequest m_deferredRequest;

    std::atomic<bool> m_cancelRequested{false};
    QPointer<QThread> m_workerThread;
    QPointer<ServiceWorker> m_worker;
};

} // namespace OpenGeoLab::App