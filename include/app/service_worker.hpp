/**
 * @file service_worker.hpp
 * @brief Background worker for asynchronous service operations
 */

#pragma once

#include "service.hpp"

#include <QObject>
#include <atomic>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::App {

/**
 * @brief Worker object that executes service operations in a background thread
 *
 * Created by BackendService and moved to a worker thread for async execution.
 * Uses nlohmann::json directly for efficient parameter handling.
 */
class ServiceWorker : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Construct a service worker
     * @param module_name Service module to invoke
     * @param params Operation parameters (pre-parsed JSON)
     * @param cancel_requested Shared cancellation flag
     * @param parent Parent QObject
     */
    explicit ServiceWorker(const QString& module_name,
                           nlohmann::json params,
                           std::atomic<bool>& cancel_requested,
                           QObject* parent = nullptr);
    ~ServiceWorker() override = default;

    [[nodiscard]] QString moduleName() const { return m_moduleName; }

public slots:
    /// Executes the service operation; emits finished or errorOccurred when done
    void process();

signals:
    void progressUpdated(double progress, const QString& message);
    void errorOccurred(const QString& module_name, const QString& error_message);
    /// @param result JSON-encoded string of operation result
    void finished(const QString& module_name, const QString& result);

private:
    QString m_moduleName;
    nlohmann::json m_params; ///< Pre-parsed JSON parameters
    std::atomic<bool>& m_cancelRequested;
};

/**
 * @brief Qt-compatible progress reporter adapter
 *
 * Bridges IProgressReporter interface to Qt signals for thread-safe UI updates.
 */
class QtProgressReporter : public IProgressReporter {
public:
    QtProgressReporter(ServiceWorker* worker, std::atomic<bool>& cancelled);

    void reportProgress(double progress, const std::string& message) override;
    void reportError(const std::string& error) override;
    [[nodiscard]] bool isCancelled() const override;

private:
    ServiceWorker* m_worker; ///< Worker to emit signals through
    std::atomic<bool>& m_cancelled;
};

} // namespace OpenGeoLab::App
