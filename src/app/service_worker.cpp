/**
 * @file service_worker.cpp
 * @brief Implementation of ServiceWorker for background service execution
 */

#include "service_worker.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/stopwatch.hpp>

namespace OpenGeoLab::App {

// =============================================================================
// QtProgressReporter Implementation
// =============================================================================

QtProgressReporter::QtProgressReporter(ServiceWorker* worker, std::atomic<bool>& cancelled)
    : m_worker(worker), m_cancelled(cancelled) {}

void QtProgressReporter::reportProgress(double progress, const std::string& message) {
    if(m_worker) {
        emit m_worker->progressUpdated(progress, QString::fromStdString(message));
    }
}

void QtProgressReporter::reportError(const std::string& error_message) {
    if(m_worker) {
        emit m_worker->errorOccurred(m_worker->moduleName(), QString::fromStdString(error_message));
    }
}

bool QtProgressReporter::isCancelled() const { return m_cancelled.load(); }

// =============================================================================
// ServiceWorker Implementation
// =============================================================================

ServiceWorker::ServiceWorker(const QString& module_name,
                             nlohmann::json params,
                             std::atomic<bool>& cancel_requested,
                             QObject* parent)
    : QObject(parent), m_moduleName(module_name), m_params(std::move(params)),
      m_cancelRequested(cancel_requested) {}

void ServiceWorker::process() {
    Kangaroo::Util::Stopwatch stopwatch("Backend [" + m_moduleName.toStdString() + "]",
                                        OpenGeoLab::getLogger());
    try {
        auto service = g_ComponentFactory.getInstanceObjectWithID<IServiceSingletonFactory>(
            m_moduleName.toStdString());

        auto reporter = std::make_shared<QtProgressReporter>(this, m_cancelRequested);

        if(m_cancelRequested.load()) {
            emit errorOccurred(m_moduleName, QStringLiteral("Operation cancelled before start."));
            return;
        }

        // Execute the service with pre-parsed JSON (no re-parsing needed)
        auto result = service->processRequest(m_moduleName.toStdString(), m_params, reporter);

        // Convert result to JSON string for signal
        emit finished(m_moduleName, QString::fromStdString(result.dump()));
    } catch(const std::exception& e) {
        emit errorOccurred(m_moduleName, QString::fromStdString(e.what()));
    }
}

} // namespace OpenGeoLab::App
