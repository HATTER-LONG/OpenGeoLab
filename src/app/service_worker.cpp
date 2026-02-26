/**
 * @file service_worker.cpp
 * @brief Implementation of ServiceWorker for background service execution
 */

#include "service_worker.hpp"
#include "service.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/stopwatch.hpp>

namespace OpenGeoLab::App {

namespace {
class QtProgressReporter final : public IProgressReporter {
public:
    QtProgressReporter(ServiceWorker* worker, std::atomic<bool>& cancelled)
        : m_worker(worker), m_cancelled(cancelled) {}

    void reportProgress(double progress, const std::string& message) override {
        if(m_worker) {
            emit m_worker->progressUpdated(progress, QString::fromStdString(message));
        }
    }

    void reportError(const std::string& error_message) override {
        if(m_worker) {
            emit m_worker->errorOccurred(m_worker->moduleName(),
                                         QString::fromStdString(error_message));
        }
    }

    [[nodiscard]] bool isCancelled() const override { return m_cancelled.load(); }

private:
    ServiceWorker* m_worker;
    std::atomic<bool>& m_cancelled;
};
} // namespace

// =============================================================================
// ServiceWorker Implementation
// =============================================================================

ServiceWorker::ServiceWorker(const QString& module_name,
                             nlohmann::json params,
                             bool silent,
                             std::atomic<bool>& cancel_requested,
                             QObject* parent)
    : QObject(parent), m_moduleName(module_name), m_params(std::move(params)), m_silent(silent),
      m_cancelRequested(cancel_requested) {}

void ServiceWorker::process() {
    std::unique_ptr<Kangaroo::Util::Stopwatch> stopwatch;
    if(!m_silent) {
        stopwatch = std::make_unique<Kangaroo::Util::Stopwatch>(
            "Backend [" + m_moduleName.toStdString() + "]", OpenGeoLab::getLogger());
    }
    try {
        auto service = g_ComponentFactory.getInstanceObjectWithID<IServiceSingletonFactory>(
            m_moduleName.toStdString());
        IProgressReporterPtr reporter;
        if(!m_silent) {
            reporter = std::make_shared<QtProgressReporter>(this, m_cancelRequested);
        }

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
