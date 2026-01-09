/**
 * @file backend_service.hpp
 * @brief QML-exposed backend service for asynchronous module operations
 */

#pragma once

#include "service.hpp"

#include <QObject>
#include <QPointer>
#include <QThread>
#include <QtQml/qqml.h>
#include <nlohmann/json.hpp>

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
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
public:
    explicit BackendService(QObject* parent = nullptr);
    ~BackendService() override;

    [[nodiscard]] bool busy() const;
    [[nodiscard]] double progress() const;
    [[nodiscard]] QString message() const;
    [[nodiscard]] QString lastError() const;

    /**
     * @brief Start an asynchronous operation
     * @param module_name Registered service module identifier
     * @param params JSON-encoded parameters for the operation
     */
    Q_INVOKABLE void request(const QString& module_name, const QString& params);

    /// Clears the last error state
    Q_INVOKABLE void clearError();

    /// Requests cancellation of the current operation
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void messageChanged();
    void lastErrorChanged();

    void operationStarted(const QString& module_name);
    void operationProgress(const QString& module_name, double progress, const QString& message);
    void operationFinished(const QString& module_name, const nlohmann::json& result);
    void operationFailed(const QString& module_name, const QString& error);

private slots:
    void onWorkerProgress(double progress, const QString& message);
    void onWorkerFinished(const QString& module_name, const nlohmann::json& result);
    void onWorkerError(const QString& module_name, const QString& error);

private:
    void setLastError(const QString& error);
    void setMessage(const QString& message);
    void setProgressInternal(double progress);
    void setBusyInternal(bool busy);
    void cleanupWorker();

private:
    bool m_busy{false};
    double m_progress{0.0};

    QString m_message;
    QString m_lastError;

    QString m_currentModuleName;

    std::atomic<bool> m_cancelRequested{false};
    QPointer<QThread> m_workerThread;
    QPointer<ServiceWorker> m_worker;
};

/**
 * @brief Worker object that executes service operations in a background thread
 *
 * Created by BackendService and moved to a worker thread for async execution.
 */
class ServiceWorker : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Construct a service worker
     * @param module_name Service module to invoke
     * @param params Operation parameters
     * @param cancel_requested Shared cancellation flag
     * @param parent Parent QObject
     */
    explicit ServiceWorker(const QString& module_name,
                           const nlohmann::json params,
                           std::atomic<bool>& cancel_requested,
                           QObject* parent = nullptr);
    ~ServiceWorker() = default;

    QString moduleName() const { return m_moduleName; }
public slots:
    /// Executes the service operation; emits finished or errorOccurred when done
    void process();

signals:
    void progressUpdated(double progress, const QString& message);
    void errorOccurred(const QString& module_name, const QString& error_message);
    void finished(const QString& module_name, const nlohmann::json& result);

private:
    QString m_moduleName;
    nlohmann::json m_params;
    std::atomic<bool>& m_cancelRequested;
};

/**
 * @brief Qt-compatible progress reporter adapter
 *
 * Bridges IProgressReporter interface to Qt signals for thread-safe UI updates.
 */
class QtProgressReporter : public App::IProgressReporter {
public:
    QtProgressReporter(ServiceWorker* worker, std::atomic<bool>& cancelled);

    void reportProgress(double progress, const std::string& message) override;
    void reportError(const std::string& error) override;
    bool isCancelled() const override;

private:
    ServiceWorker* m_worker; ///< Worker to emit signals through
    std::atomic<bool>& m_cancelled;
    QString m_lastError;
};
} // namespace OpenGeoLab::App