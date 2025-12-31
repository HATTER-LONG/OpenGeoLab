/**
 * @file backend_service.cpp
 * @brief Implementation of QML backend service with async request processing
 */
#include "backend_service.hpp"
#include "kangaroo/util/component_factory.hpp"
#include "service.hpp"
#include "util/logger.hpp"

#include <QMetaObject>

namespace OpenGeoLab {

// --- QtProgressReporter Implementation ---

QtProgressReporter::QtProgressReporter(ServiceWorker* worker, std::atomic<bool>& cancelled)
    : m_worker(worker), m_cancelled(cancelled) {}

void QtProgressReporter::reportProgress(double progress, const std::string& message) {
    if(m_worker) {
        emit m_worker->progress(progress, QString::fromStdString(message));
    }
}

void QtProgressReporter::reportError(const std::string& error) {
    m_lastError = QString::fromStdString(error);
}

bool QtProgressReporter::isCancelled() const { return m_cancelled.load(); }

// --- ServiceWorker Implementation ---

ServiceWorker::ServiceWorker(const QString& module,
                             const QString& action_id,
                             const nlohmann::json& params,
                             std::atomic<bool>& cancelled)
    : m_module(module), m_actionId(action_id), m_params(params), m_cancelled(cancelled) {}

void ServiceWorker::process() {
    try {
        auto service = g_ComponentFactory.getInstanceObjectWithID<App::ServiceBaseSingletonFactory>(
            m_module.toStdString());

        if(!service) {
            emit error(m_actionId, QStringLiteral("Service module not found: %1").arg(m_module));
            return;
        }

        auto reporter = std::make_shared<QtProgressReporter>(this, m_cancelled);
        nlohmann::json result =
            service->processRequest(m_actionId.toStdString(), m_params, reporter);

        if(m_cancelled.load()) {
            emit error(m_actionId, QStringLiteral("Operation cancelled"));
            return;
        }

        QVariantMap result_map = BackendService::jsonToVariantMap(result);
        emit finished(m_actionId, result_map);

    } catch(const std::exception& e) {
        LOG_ERROR("Service request failed: {}", e.what());
        emit error(m_actionId, QString::fromStdString(e.what()));
    }
}

// --- BackendService Implementation ---

BackendService::BackendService(QObject* parent) : QObject(parent) {}

BackendService::~BackendService() { cleanupWorker(); }

bool BackendService::busy() const { return m_busy; }

double BackendService::progress() const { return m_progress; }

QString BackendService::message() const { return m_message; }

QString BackendService::lastError() const { return m_lastError; }

void BackendService::request(const QString& action_id, const QVariantMap& params) {
    clearError();

    if(action_id.trimmed().isEmpty()) {
        setLastError(QStringLiteral("action_id is empty"));
        emit operationFailed(action_id, m_lastError);
        return;
    }

    if(m_busy) {
        const auto err = QStringLiteral("BackendService is busy");
        setLastError(err);
        emit operationFailed(action_id, err);
        return;
    }

    // Extract module from params for routing
    QString module = params.value(QStringLiteral("module")).toString();
    if(module.isEmpty()) {
        setLastError(QStringLiteral("Missing 'module' parameter for routing"));
        emit operationFailed(action_id, m_lastError);
        return;
    }

    m_currentActionId = action_id;
    m_currentParams = params;
    m_cancelled.store(false);

    setBusyInternal(true);
    setProgressInternal(-1.0); // Indeterminate initially
    setMessage(QStringLiteral("%1...").arg(action_id));

    emit operationStarted(action_id);

    // Convert QVariantMap to JSON for service layer
    nlohmann::json json_params = variantMapToJson(params);

    // Create worker thread for async processing
    cleanupWorker();

    m_workerThread = new QThread(this);
    m_worker = new ServiceWorker(module, action_id, json_params, m_cancelled);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &ServiceWorker::process);
    connect(m_worker, &ServiceWorker::progress, this, &BackendService::onWorkerProgress);
    connect(m_worker, &ServiceWorker::finished, this, &BackendService::onWorkerFinished);
    connect(m_worker, &ServiceWorker::error, this, &BackendService::onWorkerError);

    connect(m_worker, &ServiceWorker::finished, m_workerThread, &QThread::quit);
    connect(m_worker, &ServiceWorker::error, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();
}

void BackendService::onWorkerProgress(double progress, const QString& message) {
    setProgressInternal(progress);
    if(!message.isEmpty()) {
        setMessage(message);
    }
    emit operationProgress(m_currentActionId, progress, m_message);
}

void BackendService::onWorkerFinished(const QString& action_id, const QVariantMap& result) {
    setBusyInternal(false);
    setProgressInternal(1.0);
    setMessage(QString());

    emit operationFinished(action_id, result);

    m_currentActionId.clear();
    m_currentParams.clear();
}

void BackendService::onWorkerError(const QString& action_id, const QString& error) {
    setBusyInternal(false);
    setProgressInternal(-1.0);
    setMessage(QString());
    setLastError(error);

    emit operationFailed(action_id, error);

    m_currentActionId.clear();
    m_currentParams.clear();
}

void BackendService::setBusy(bool busy, const QString& message) {
    setBusyInternal(busy);
    if(!message.isEmpty()) {
        setMessage(message);
    }

    if(!busy) {
        cleanupWorker();
        m_currentActionId.clear();
        m_currentParams.clear();
    }
}

void BackendService::setProgress(double progress, const QString& message) {
    setProgressInternal(progress);
    if(!message.isEmpty()) {
        setMessage(message);
    }
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

    const auto action_id = m_currentActionId;
    m_cancelled.store(true);

    // Wait for worker to finish gracefully
    if(m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(1000);
    }

    cleanupWorker();

    setBusyInternal(false);
    setProgressInternal(-1.0);
    setMessage(QString());

    const auto err = QStringLiteral("Cancelled");
    setLastError(err);

    emit operationFailed(action_id, err);

    m_currentActionId.clear();
    m_currentParams.clear();
}

void BackendService::setLastError(const QString& error) {
    if(m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

void BackendService::setMessage(const QString& message) {
    if(m_message == message) {
        return;
    }
    m_message = message;
    emit messageChanged();
}

void BackendService::setProgressInternal(double progress) {
    if(qFuzzyCompare(m_progress, progress)) {
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

nlohmann::json BackendService::variantMapToJson(const QVariantMap& map) { // NOLINT
    nlohmann::json json = nlohmann::json::object();

    for(auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();

        switch(value.typeId()) {
        case QMetaType::Bool:
            json[key.toStdString()] = value.toBool();
            break;
        case QMetaType::Int:
            json[key.toStdString()] = value.toInt();
            break;
        case QMetaType::LongLong:
            json[key.toStdString()] = value.toLongLong();
            break;
        case QMetaType::Double:
            json[key.toStdString()] = value.toDouble();
            break;
        case QMetaType::QString:
            json[key.toStdString()] = value.toString().toStdString();
            break;
        case QMetaType::QVariantList: {
            nlohmann::json arr = nlohmann::json::array();
            const QVariantList list = value.toList();
            for(const auto& item : list) {
                if(item.typeId() == QMetaType::QString) {
                    arr.push_back(item.toString().toStdString());
                } else if(item.typeId() == QMetaType::Double) {
                    arr.push_back(item.toDouble());
                } else if(item.typeId() == QMetaType::Int) {
                    arr.push_back(item.toInt());
                } else if(item.typeId() == QMetaType::Bool) {
                    arr.push_back(item.toBool());
                }
            }
            json[key.toStdString()] = arr;
            break;
        }
        case QMetaType::QVariantMap:
            json[key.toStdString()] = variantMapToJson(value.toMap());
            break;
        default:
            // Fallback: convert to string
            json[key.toStdString()] = value.toString().toStdString();
            break;
        }
    }

    return json;
}

QVariantMap BackendService::jsonToVariantMap(const nlohmann::json& json) { // NOLINT
    QVariantMap map;

    if(!json.is_object()) {
        return map;
    }

    for(auto it = json.begin(); it != json.end(); ++it) {
        const QString key = QString::fromStdString(it.key());
        const auto& value = it.value();

        if(value.is_null()) {
            map[key] = QVariant();
        } else if(value.is_boolean()) {
            map[key] = value.get<bool>();
        } else if(value.is_number_integer()) {
            map[key] = static_cast<qlonglong>(value.get<int64_t>());
        } else if(value.is_number_float()) {
            map[key] = value.get<double>();
        } else if(value.is_string()) {
            map[key] = QString::fromStdString(value.get<std::string>());
        } else if(value.is_array()) {
            QVariantList list;
            for(const auto& item : value) {
                if(item.is_string()) {
                    list.append(QString::fromStdString(item.get<std::string>()));
                } else if(item.is_number_float()) {
                    list.append(item.get<double>());
                } else if(item.is_number_integer()) {
                    list.append(static_cast<qlonglong>(item.get<int64_t>()));
                } else if(item.is_boolean()) {
                    list.append(item.get<bool>());
                } else if(item.is_object()) {
                    list.append(jsonToVariantMap(item));
                }
            }
            map[key] = list;
        } else if(value.is_object()) {
            map[key] = jsonToVariantMap(value);
        }
    }

    return map;
}

} // namespace OpenGeoLab
