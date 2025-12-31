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
     * @param action_id Action identifier (e.g., "read_model")
     * @param params Parameters including "module" key for routing
     * @note Expects params["module"] to specify target service component
     */
    Q_INVOKABLE void request(const QString& action_id, const QVariantMap& params);

    Q_INVOKABLE void setBusy(bool busy, const QString& message = QString());
    Q_INVOKABLE void setProgress(double progress, const QString& message = QString());
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void messageChanged();
    void lastErrorChanged();

    void operationStarted(const QString& action_id);
    void operationProgress(const QString& action_id, double progress, const QString& message);
    void operationFinished(const QString& action_id, const QVariantMap& result);
    void operationFailed(const QString& action_id, const QString& error);

private slots:
    void onWorkerProgress(double progress, const QString& message);
    void onWorkerFinished(const QString& action_id, const QVariantMap& result);
    void onWorkerError(const QString& action_id, const QString& error);

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

    QString m_currentActionId;
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
    ServiceWorker(const QString& module,
                  const QString& action_id,
                  const nlohmann::json& params,
                  std::atomic<bool>& cancelled);

public slots:
    void process();

signals:
    void progress(double progress, const QString& message);
    void finished(const QString& action_id, const QVariantMap& result);
    void error(const QString& action_id, const QString& error);

private:
    QString m_module;
    QString m_actionId;
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
