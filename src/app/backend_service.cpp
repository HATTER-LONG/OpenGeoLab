#include "backend_service.hpp"
#include "nlohmann/json.hpp"
#include "service.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>
namespace OpenGeoLab {
namespace App {

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

ServiceWorker::ServiceWorker(const QString& module_name,
                             const nlohmann::json params,
                             std::atomic<bool>& cancel_requested,
                             QObject* parent)
    : QObject(parent), m_moduleName(module_name), m_params(params),
      m_cancelRequested(cancel_requested) {}

void ServiceWorker::process() {
    try {
        auto service = g_ComponentFactory.getInstanceObjectWithID<App::IServiceSigletonFactory>(
            m_moduleName.toStdString());
        auto report = std::make_shared<QtProgressReporter>(this, m_cancelRequested);
        if(m_cancelRequested.load()) {
            emit errorOccurred(m_moduleName, "Operation cancelled before start.");
            return;
        }
        auto result = service->processRequest(m_moduleName.toStdString(), m_params, report);
        emit finished(m_moduleName, result);
    } catch(const std::exception& e) {
        LOG_ERROR("ServiceWorker[{}] encountered an error: {}", m_moduleName.toStdString(),
                  e.what());
        emit errorOccurred(m_moduleName, QString::fromStdString(e.what()));
    }
}

BackendService::BackendService(QObject* parent) : QObject(parent) {}
BackendService::~BackendService() { cleanupWorker(); }

bool BackendService::busy() const { return m_busy; }
double BackendService::progress() const { return m_progress; }
QString BackendService::message() const { return m_message; }
QString BackendService::lastError() const { return m_lastError; }

void BackendService::request(const QString& module_name, const QString& params) {
    LOG_INFO("BackendService::request called with module: {}", qPrintable(module_name));

    clearError();

    if(module_name.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Module name is empty. Aborting request."));
        emit operationFailed(module_name, m_lastError);
        return;
    }

    if(m_busy) {
        const auto err = QStringLiteral("Service is currently busy. Cannot process new request.");
        setLastError(err);
        emit operationFailed(module_name, err);
        return;
    }
    setBusyInternal(true);

    nlohmann::json param_json;
    try {
        param_json = nlohmann::json::parse(params.toStdString());
        LOG_DEBUG("params JSON: {}", param_json.dump(4));
    } catch(const nlohmann::json::parse_error& e) {
        setLastError(QStringLiteral("Failed to parse params JSON: ") +
                     QString::fromStdString(e.what()));
        emit operationFailed(module_name, m_lastError);
        return;
    }

    m_currentModuleName = module_name;
    m_cancelRequested.store(false);

    setProgressInternal(0.0);
    setMessage(QStringLiteral("Starting operation...%1").arg(module_name));
    emit operationStarted(module_name);
    cleanupWorker();
    m_workerThread = new QThread(this);
    m_worker = new ServiceWorker(module_name, param_json, m_cancelRequested);
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::started, m_worker, &ServiceWorker::process);
    connect(m_worker, &ServiceWorker::progressUpdated, this, &BackendService::onWorkerProgress);
    connect(m_worker, &ServiceWorker::finished, this, &BackendService::onWorkerFinished);
    connect(m_worker, &ServiceWorker::errorOccurred, this, &BackendService::onWorkerError);

    connect(m_worker, &ServiceWorker::finished, m_workerThread, &QThread::quit);
    connect(m_worker, &ServiceWorker::errorOccurred, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_workerThread->start();
}

void BackendService::clearError() {
    if(m_lastError.isEmpty()) {
        return;
    }
    m_lastError.clear();
    emit lastErrorChanged();
}

void BackendService::cancel() {
    if(!m_busy) {
        return;
    }
    m_cancelRequested.store(true);
    setMessage(
        QStringLiteral("Cancellation requested for operation %1...").arg(m_currentModuleName));
    if(m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(1000);
    }
    if(std::abs(m_progress - 1.0) > 1e-6) {
        cleanupWorker();
        setBusyInternal(false);
        setProgressInternal(0.0);
        setMessage(QStringLiteral("Operation %1 cancelled.").arg(m_currentModuleName));
        setLastError(QStringLiteral("Operation cancelled by user."));

        emit operationFailed(m_currentModuleName, m_lastError);
    }
}

void BackendService::onWorkerProgress(double progress, const QString& message) {
    setProgressInternal(progress);
    if(!message.isEmpty()) {
        setMessage(message);
    }
    emit operationProgress(m_currentModuleName, progress, message);
}

void BackendService::onWorkerFinished(const QString& module_name, const nlohmann::json& result) {
    setProgressInternal(1.0);
    setMessage(QStringLiteral("Operation %1 completed successfully.").arg(module_name));
    emit operationFinished(module_name, result);
    cleanupWorker();
    setBusyInternal(false);
}

void BackendService::onWorkerError(const QString& module_name, const QString& error) {
    setLastError(error);
    setProgressInternal(0.0);
    setMessage(QStringLiteral("Operation %1 failed: %2").arg(module_name, error));
    emit operationFailed(module_name, error);
    cleanupWorker();
    setBusyInternal(false);
}

void BackendService::setLastError(const QString& error) {
    LOG_ERROR("BackendService encountered an error: {}", qPrintable(error));
    m_lastError = error;
    emit lastErrorChanged();
}
void BackendService::setMessage(const QString& message) {
    m_message = message;
    emit messageChanged();
}

void BackendService::setProgressInternal(double progress) {
    m_progress = progress;
    emit progressChanged();
}

void BackendService::setBusyInternal(bool busy) {
    if(m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void BackendService::cleanupWorker() {
    if(m_workerThread) {
        if(m_workerThread->isRunning()) {
            m_workerThread->quit();
            m_workerThread->wait();
        }
        m_workerThread->deleteLater();
        m_workerThread.clear();
    }
    m_worker.clear();
}
} // namespace App
} // namespace OpenGeoLab