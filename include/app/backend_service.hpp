/**
 * @file backend_service.hpp
 * @brief QML-exposed backend service entry point for async operations
 */
#pragma once

#include "service.hpp"

#include <QObject>
#include <QPointer>
#include <QThread>
#include <QVariantMap>

#include <QtQml/qqml.h>

#include <QString>

#include <atomic>

namespace OpenGeoLab {

class ServiceWorker;

/**
 * @brief Main backend service interface exposed to QML
 *
 * Provides async request processing with progress reporting.
 * Routes requests to appropriate service components based on module parameter.
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
     * @brief Process a backend request asynchronously
     * @param module_name Module/service name for routing (e.g., "ModelReader")
     * @param params Request parameters specific to the module
     */
    Q_INVOKABLE void request(const QString& module_name, const QVariantMap& params);

    Q_INVOKABLE void setBusy(bool busy, const QString& message = QString());
    Q_INVOKABLE void setProgress(double progress, const QString& message = QString());
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void messageChanged();
    void lastErrorChanged();

    void operationStarted(const QString& module_name);
    void operationProgress(const QString& module_name, double progress, const QString& message);
    void operationFinished(const QString& module_name, const QVariantMap& result);
    void operationFailed(const QString& module_name, const QString& error);

private slots:
    void onWorkerProgress(double progress, const QString& message);
    void onWorkerFinished(const QString& module_name, const QVariantMap& result);
    void onWorkerError(const QString& module_name, const QString& error);

public:
    /**
     * @brief Convert QVariantMap to nlohmann::json
     * @param map Qt variant map from QML
     * @return JSON object representation
     */
    static nlohmann::json variantMapToJson(const QVariantMap& map);

    /**
     * @brief Convert nlohmann::json to QVariantMap
     * @param json JSON object from service layer
     * @return Qt variant map for QML
     */
    static QVariantMap jsonToVariantMap(const nlohmann::json& json);

private:
    void setLastError(const QString& error);
    void setMessage(const QString& message);
    void setProgressInternal(double progress);
    void setBusyInternal(bool busy);
    void cleanupWorker();

    bool m_busy = false;
    double m_progress = -1.0;
    QString m_message;
    QString m_lastError;

    QString m_currentModuleName;
    QVariantMap m_currentParams;

    std::atomic<bool> m_cancelled{false};
    QPointer<QThread> m_workerThread;
    QPointer<ServiceWorker> m_worker;
};

/**
 * @brief Worker object for executing service requests in background thread
 */
class ServiceWorker : public QObject {
    Q_OBJECT

public:
    ServiceWorker(const QString& module_name,
                  const nlohmann::json& params,
                  std::atomic<bool>& cancelled);

public slots:
    void process();

signals:
    void progress(double progress, const QString& message);
    void finished(const QString& module_name, const QVariantMap& result);
    void error(const QString& module_name, const QString& error);

private:
    QString m_moduleName;
    nlohmann::json m_params;
    std::atomic<bool>& m_cancelled;
};

/**
 * @brief Progress reporter implementation that bridges to Qt signals
 */
class QtProgressReporter : public App::IProgressReporter {
public:
    QtProgressReporter(ServiceWorker* worker, std::atomic<bool>& cancelled);

    void reportProgress(double progress, const std::string& message) override;
    void reportError(const std::string& error) override;
    bool isCancelled() const override;

private:
    ServiceWorker* m_worker;
    std::atomic<bool>& m_cancelled;
    QString m_lastError;
};

} // namespace OpenGeoLab
