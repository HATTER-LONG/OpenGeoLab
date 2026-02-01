/**
 * @file backend_service.cpp
 * @brief Implementation of BackendService for asynchronous operations
 */

#include "backend_service.hpp"
#include "service_worker.hpp"
#include "util/logger.hpp"
#include "util/progress_callback.hpp"

#include <nlohmann/json.hpp>

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {

namespace {
class NullProgressReporter final : public IProgressReporter {
public:
    void reportProgress(double /*progress*/, const std::string& /*message*/) override {}
    void reportError(const std::string& /*error_message*/) override {}
    [[nodiscard]] bool isCancelled() const override { return false; }
};

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
} // namespace

BackendService::BackendService(QObject* parent) : QObject(parent) {}
BackendService::~BackendService() { cleanupWorker(); }

bool BackendService::busy() const { return m_busy; }
double BackendService::progress() const { return m_progress; }
QString BackendService::message() const { return m_message; }

void BackendService::request(const QString& module_name, const QString& params) {
    if(module_name.trimmed().isEmpty()) {
        emit operationFailed(module_name,
                             QStringLiteral("Module name is empty. Aborting request."));
        return;
    }

    // Parse JSON once here, then pass the parsed object to the worker
    nlohmann::json param_json;
    if(!params.trimmed().isEmpty()) {
        try {
            param_json = nlohmann::json::parse(params.toStdString());
        } catch(const nlohmann::json::parse_error& e) {
            emit operationFailed(module_name, QStringLiteral("Failed to parse params JSON: ") +
                                                  QString::fromStdString(e.what()));
            return;
        }
    }

    const bool silent = isSilentRequest(param_json);
    if(!silent) {
        LOG_DEBUG("Backend request: module='{}'", qPrintable(module_name));
        if(param_json.is_object() && !param_json.empty()) {
            LOG_TRACE("Backend params: {}", param_json.dump(4));
        }
    }

    // Silent requests are intended for frequent UI actions (e.g., view changes).
    // They execute synchronously without progress overlay/log spam.
    if(silent) {
        try {
            auto service = g_ComponentFactory.getInstanceObjectWithID<IServiceSingletonFactory>(
                module_name.toStdString());
            if(!service) {
                throw std::runtime_error("Service factory not found for module: " +
                                         module_name.toStdString());
            }

            auto reporter = std::make_shared<NullProgressReporter>();
            (void)service->processRequest(module_name.toStdString(), param_json, reporter);
        } catch(const std::exception& e) {
            // Keep errors visible to logs, but don't trigger UI progress overlay.
            LOG_ERROR("Silent backend error [{}]: {}", qPrintable(module_name), e.what());
        }
        return;
    }

    if(m_busy) {
        const auto err = QStringLiteral("Service is currently busy. Cannot process new request.");
        emit operationFailed(module_name, err);
        return;
    }
    setBusyInternal(true);

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