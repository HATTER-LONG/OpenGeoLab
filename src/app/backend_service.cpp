/**
 * @file backend_service.cpp
 * @brief Implementation of BackendService for asynchronous operations
 */

#include "backend_service.hpp"
#include "service_worker.hpp"
#include "util/logger.hpp"

#include <QMetaObject>

#include <cmath>

#include <nlohmann/json.hpp>

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {

namespace {
[[nodiscard]] bool isSilentRequest(const nlohmann::json& params) {
    if(!params.is_object()) {
        return false;
    }
    if(!params.contains("_meta") || !params["_meta"].is_object()) {
        return false;
    }
    const auto& meta = params["_meta"];
    return meta.contains("silent") && meta["silent"].is_boolean() && meta["silent"].get<bool>();
}

[[nodiscard]] bool isDeferIfBusyRequest(const nlohmann::json& params) {
    if(!params.is_object()) {
        return false;
    }
    if(!params.contains("_meta") || !params["_meta"].is_object()) {
        return false;
    }
    const auto& meta = params["_meta"];
    return meta.contains("defer_if_busy") && meta["defer_if_busy"].is_boolean() &&
           meta["defer_if_busy"].get<bool>();
}

[[nodiscard]] QString extractActionName(const nlohmann::json& params) {
    if(!params.is_object()) {
        return {};
    }
    auto it = params.find("action");
    if(it == params.end()) {
        return {};
    }
    if(it->is_string()) {
        const auto value = QString::fromStdString(it->get<std::string>()).trimmed();
        if(!value.isEmpty()) {
            return value;
        }
    }
    return {};
}
} // namespace

BackendService::BackendService(QObject* parent) : QObject(parent) {}
BackendService::~BackendService() { cleanupWorker(); }

bool BackendService::busy() const { return m_busy; }
double BackendService::progress() const { return m_progress; }
QString BackendService::message() const { return m_message; }

void BackendService::request(const QString& module_name, const QString& params) {
    if(module_name.trimmed().isEmpty()) {
        emit operationFailed(module_name, QStringLiteral("Module name is empty. Aborting request."),
                             QString{});
        return;
    }

    // Parse JSON once here, then pass the parsed object to the worker
    nlohmann::json param_json;
    if(!params.trimmed().isEmpty()) {
        try {
            param_json = nlohmann::json::parse(params.toStdString());
        } catch(const nlohmann::json::parse_error& e) {
            emit operationFailed(module_name,
                                 QStringLiteral("Failed to parse params JSON: ") +
                                     QString::fromStdString(e.what()),
                                 QString{});
            return;
        }
    }

    const auto current_action_name = extractActionName(param_json);

    if(m_processingRequest) {
        if(isDeferIfBusyRequest(param_json)) {
            m_deferredRequest.m_pending = true;
            m_deferredRequest.m_moduleName = module_name;
            m_deferredRequest.m_params = params;
            LOG_DEBUG("Backend busy, deferred request queued: module='{}', action='{}'",
                      qPrintable(module_name), qPrintable(current_action_name));
            return;
        }
        const auto err = QStringLiteral("Service is currently busy. Cannot process new request.");
        emit operationFailed(module_name, current_action_name, err);
        return;
    }

    const bool silent = isSilentRequest(param_json);
    m_processingRequest = true;
    m_currentRequest.m_moduleName = module_name;
    m_currentRequest.m_actionName = current_action_name;
    m_currentRequest.m_silent = silent;

    if(!silent) {
        LOG_DEBUG("Backend request: module='{}'", qPrintable(module_name));
        if(param_json.is_object() && !param_json.empty()) {
            LOG_TRACE("Backend params: {}", param_json.dump(4));
        }
    }

    if(!silent) {
        setBusyInternal(true);
    }
    m_cancelRequested.store(false);

    if(!silent) {
        setProgressInternal(0.0);
        setMessage(QStringLiteral("Starting operation...[%1]").arg(module_name));
        emit operationStarted(module_name, m_currentRequest.m_actionName);
    }

    cleanupWorker();
    m_workerThread = new QThread(this);
    // Pass pre-parsed JSON to worker (avoids re-parsing in worker thread)
    m_worker = new ServiceWorker(module_name, std::move(param_json), silent, m_cancelRequested);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &ServiceWorker::process);
    connect(m_worker, &ServiceWorker::progressUpdated, this, &BackendService::onWorkerProgress);
    connect(m_worker, &ServiceWorker::finished, this, &BackendService::onWorkerFinished);
    connect(m_worker, &ServiceWorker::errorOccurred, this, &BackendService::onWorkerError);

    connect(m_worker, &ServiceWorker::finished, m_workerThread, &QThread::quit);
    connect(m_worker, &ServiceWorker::errorOccurred, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, this, &BackendService::onWorkerThreadFinished);

    m_workerThread->start();
}

void BackendService::cancel() {
    if(!m_processingRequest) {
        return;
    }
    m_cancelRequested.store(true);
    if(!m_currentRequest.m_silent) {
        setMessage(QStringLiteral("Cancellation requested for operation [%1]...")
                       .arg(m_currentRequest.m_moduleName));
    }
    if(m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(1000);
    }
}

void BackendService::onWorkerProgress(double progress, const QString& message) {
    if(m_currentRequest.m_silent) {
        return;
    }
    setProgressInternal(progress);
    if(!message.isEmpty()) {
        setMessage(message);
    }
    emit operationProgress(m_currentRequest.m_moduleName, m_currentRequest.m_actionName, progress,
                           message);
}

void BackendService::onWorkerFinished(const QString& module_name, const QString& result) {
    const auto request_context = m_currentRequest;
    if(!request_context.m_silent) {
        setProgressInternal(1.0);
        setMessage(QStringLiteral("Operation [%1] completed successfully.").arg(module_name));
        setBusyInternal(false);
    }
    m_processingRequest = false;
    m_currentRequest.reset();

    emit operationFinished(module_name, request_context.m_actionName, result);
    if(request_context.m_silent) {
        LOG_DEBUG("Silent backend request [{}] completed successfully.", qPrintable(module_name));
    } else {
        LOG_INFO("Backend operation [{}] finished successfully.", qPrintable(module_name));
    }
}

void BackendService::onWorkerError(const QString& module_name, const QString& error) {
    const auto request_context = m_currentRequest;
    QString final_error = error;
    if(request_context.m_silent) {
        final_error = QStringLiteral("Silent request error: ") + error;
    } else {
        setProgressInternal(0.0);
        setMessage(QStringLiteral("Operation [%1] failed: %2").arg(module_name, error));
        setBusyInternal(false);
    }
    m_processingRequest = false;
    m_currentRequest.reset();

    emit operationFailed(module_name, request_context.m_actionName, final_error);
    if(request_context.m_silent) {
        LOG_ERROR("Silent backend error [{}]: {}", qPrintable(module_name), qPrintable(error));
    } else {
        LOG_ERROR("Backend error [{}]: {}", qPrintable(module_name), qPrintable(error));
    }
}

void BackendService::onWorkerThreadFinished() {
    m_worker.clear();
    m_workerThread.clear();
    scheduleDeferredRequestIfNeeded();
}

void BackendService::setMessage(const QString& message) {
    if(m_message == message) {
        return;
    }
    m_message = message;
    emit messageChanged();
}

void BackendService::setProgressInternal(double progress) {
    constexpr double progress_epsilon = 1e-4;
    if(std::abs(m_progress - progress) < progress_epsilon) {
        return;
    }
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

void BackendService::scheduleDeferredRequestIfNeeded() {
    if(!m_processingRequest && m_deferredRequest.m_pending) {
        QMetaObject::invokeMethod(
            this, [this]() { processDeferredRequest(); }, Qt::QueuedConnection);
    }
}

void BackendService::processDeferredRequest() {
    if(m_processingRequest || !m_deferredRequest.m_pending) {
        return;
    }

    const auto deferred_module_name = m_deferredRequest.m_moduleName;
    const auto deferred_params = m_deferredRequest.m_params;
    m_deferredRequest.reset();

    if(deferred_module_name.trimmed().isEmpty()) {
        return;
    }

    request(deferred_module_name, deferred_params);
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