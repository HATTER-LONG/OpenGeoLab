/**
 * @file backend_service.cpp
 * @brief Implementation of BackendService for asynchronous operations
 */

#include "backend_service.hpp"
#include "service_worker.hpp"
#include "util/logger.hpp"

#include <nlohmann/json.hpp>

namespace OpenGeoLab::App {

BackendService::BackendService(QObject* parent) : QObject(parent) {}
BackendService::~BackendService() { cleanupWorker(); }

bool BackendService::busy() const { return m_busy; }
double BackendService::progress() const { return m_progress; }
QString BackendService::message() const { return m_message; }

void BackendService::request(const QString& module_name, const QString& params) {
    LOG_DEBUG("Backend request: module='{}'", qPrintable(module_name));

    if(module_name.trimmed().isEmpty()) {
        emit operationFailed(module_name,
                             QStringLiteral("Module name is empty. Aborting request."));
        return;
    }

    if(m_busy) {
        const auto err = QStringLiteral("Service is currently busy. Cannot process new request.");
        emit operationFailed(module_name, err);
        return;
    }
    setBusyInternal(true);

    // Parse JSON once here, then pass the parsed object to the worker
    nlohmann::json param_json;
    if(!params.trimmed().isEmpty()) {
        try {
            param_json = nlohmann::json::parse(params.toStdString());
            LOG_TRACE("Backend params: {}", param_json.dump(4));
        } catch(const nlohmann::json::parse_error& e) {
            setBusyInternal(false);
            emit operationFailed(module_name, QStringLiteral("Failed to parse params JSON: ") +
                                                  QString::fromStdString(e.what()));
            return;
        }
    }

    m_currentModuleName = module_name;
    m_cancelRequested.store(false);

    setProgressInternal(0.0);
    setMessage(QStringLiteral("Starting operation...[%1]").arg(module_name));
    emit operationStarted(module_name);

    cleanupWorker();
    m_workerThread = new QThread(this);
    // Pass pre-parsed JSON to worker (avoids re-parsing in worker thread)
    m_worker = new ServiceWorker(module_name, std::move(param_json), m_cancelRequested);
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

void BackendService::cancel() {
    if(!m_busy) {
        return;
    }
    m_cancelRequested.store(true);
    setMessage(
        QStringLiteral("Cancellation requested for operation [%1]...").arg(m_currentModuleName));
    if(m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(1000);
    }
}

void BackendService::onWorkerProgress(double progress, const QString& message) {
    setProgressInternal(progress);
    if(!message.isEmpty()) {
        setMessage(message);
    }
    emit operationProgress(m_currentModuleName, progress, message);
}

void BackendService::onWorkerFinished(const QString& module_name, const QString& result) {
    setProgressInternal(1.0);
    setMessage(QStringLiteral("Operation [%1] completed successfully.").arg(module_name));
    emit operationFinished(module_name, result);
    cleanupWorker();
    LOG_INFO("Backend operation [{}] finished successfully.", qPrintable(module_name));
    setBusyInternal(false);
}

void BackendService::onWorkerError(const QString& module_name, const QString& error) {
    setProgressInternal(0.0);
    setMessage(QStringLiteral("Operation [%1] failed: %2").arg(module_name, error));
    emit operationFailed(module_name, error);
    cleanupWorker();
    LOG_ERROR("Backend error [{}]: {}", qPrintable(module_name), qPrintable(error));
    setBusyInternal(false);
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

} // namespace OpenGeoLab::App