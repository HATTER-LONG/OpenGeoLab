#include <opengeolab/app/request_service.hpp>

#include <opengeolab/app/progress_tracker.hpp>
#include <opengeolab/python/embedded_python_runtime.hpp>

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QtConcurrent>

#include <exception>

namespace OpenGeoLab::App {

RequestService::RequestService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
                               ProgressTracker& progress_tracker,
                               QObject* parent)
    : QObject(parent), runtime_(runtime), progress_tracker_(progress_tracker) {}

RequestService::~RequestService() {
    const std::lock_guard lock(futures_mutex_);
    for(auto& future : pending_futures_) {
        future.waitForFinished();
    }
}

QString RequestService::submitAsync(const QString& request_json) {
    const auto request_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto description = extractDescription(request_json);
    const auto injected_json = injectRequestId(request_json, request_id);

    pending_count_.fetch_add(1, std::memory_order_relaxed);
    emit busyChanged();
    progress_tracker_.beginTask(request_id, description);

    auto* watcher = new QFutureWatcher<QString>(this);

    auto future = QtConcurrent::run(
        [this, json = injected_json.toStdString(), task_id = request_id]() -> QString {
            OpenGeoLab::Python::ProgressCallback progress_cb =
                [tracker = &progress_tracker_, task_id](double progress, std::string_view message) {
                    tracker->updateProgress(
                        task_id, progress,
                        QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
                };
            return QString::fromStdString(runtime_.process(json, std::move(progress_cb)));
        });

    {
        const std::lock_guard lock(futures_mutex_);
        pending_futures_.push_back(future);
    }

    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, request_id]() {
        pending_count_.fetch_sub(1, std::memory_order_relaxed);
        emit busyChanged();

        try {
            const QString response = watcher->result();
            progress_tracker_.completeTask(request_id, true);
            emitResponse(request_id, response);
        } catch(const std::exception& exception) {
            progress_tracker_.completeTask(request_id, false);
            emit errorOccurred(request_id, QString::fromStdString(exception.what()));
        }

        {
            const std::lock_guard lock(futures_mutex_);
            std::erase_if(pending_futures_,
                          [](const QFuture<QString>& future) { return future.isFinished(); });
        }

        watcher->deleteLater();
    });

    watcher->setFuture(future);
    return request_id;
}

QString RequestService::executeOnMainThread(const QString& request_json) {
    const auto request_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto description = extractDescription(request_json);
    const auto injected_json = injectRequestId(request_json, request_id);

    progress_tracker_.beginTask(request_id, description);

    OpenGeoLab::Python::ProgressCallback progress_cb =
        [this, request_id](double progress, std::string_view message) {
            progress_tracker_.updateProgress(
                request_id, progress,
                QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
        };

    try {
        // Main thread GIL note: main.cpp releases GIL before app.exec().
        // runtime_.process() re-acquires the GIL internally, executes Python, then releases.
        // This is intentional for PySide6 invoke_ui — Qt widget creation must happen
        // on the main thread. Keep operations short (e.g. show() a window) to avoid
        // blocking the event loop.
        const auto response =
            QString::fromStdString(runtime_.process(injected_json.toStdString(), std::move(progress_cb)));
        progress_tracker_.completeTask(request_id, true);
        emitResponse(request_id, response);
    } catch(const std::exception& exception) {
        progress_tracker_.completeTask(request_id, false);
        emit errorOccurred(request_id, QString::fromStdString(exception.what()));
    }

    return request_id;
}

bool RequestService::isBusy() const { return pending_count_.load(std::memory_order_relaxed) > 0; }

QString RequestService::injectRequestId(const QString& json, const QString& request_id) {
    auto document = QJsonDocument::fromJson(json.toUtf8());
    auto object = document.object();
    object.insert(QStringLiteral("requestId"), request_id);
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QString RequestService::extractDescription(const QString& json) {
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    const auto object = document.object();
    const auto module = object.value("module").toString(QStringLiteral("unknown"));
    const auto action = object.value("action").toString(QStringLiteral("unknown"));
    return QStringLiteral("%1.%2").arg(module, action);
}

void RequestService::emitResponse(const QString& request_id, const QString& response) {
    const auto document = QJsonDocument::fromJson(response.toUtf8());
    const auto object = document.object();
    if(object.value("ok").toBool(false)) {
        emit responseReady(request_id, response);
    } else {
        const auto summary = object.value("summary").toString(QStringLiteral("Unknown error"));
        emit errorOccurred(request_id, summary);
    }
}

} // namespace OpenGeoLab::App
