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
    : QObject(parent), m_runtime(runtime), m_progressTracker(progress_tracker) {}

RequestService::~RequestService() {
    const std::lock_guard lock(m_futuresMutex);
    for(auto& future : m_pendingFutures) {
        future.waitForFinished();
    }
}

QString RequestService::submitAsync(const QString& request_json) {
    auto [request_id, description, injected_json, muted] = prepareRequest(request_json);

    m_pendingCount.fetch_add(1, std::memory_order_relaxed);
    emit busyChanged();
    if(!muted) {
        m_progressTracker.beginTask(request_id, description);
    }

    auto* watcher = new QFutureWatcher<QString>(this);

    auto future = QtConcurrent::run(
        [this, json = injected_json.toStdString(), task_id = request_id, muted]() -> QString {
            OpenGeoLab::Python::ProgressCallback progress_cb;
            if(!muted) {
                progress_cb = [tracker = &m_progressTracker, task_id](double progress,
                                                                      std::string_view message) {
                    tracker->updateProgress(
                        task_id, progress,
                        QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
                };
            }
            return QString::fromStdString(m_runtime.process(json, std::move(progress_cb)));
        });

    {
        const std::lock_guard lock(m_futuresMutex);
        m_pendingFutures.push_back(future);
    }

    connect(
        watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, request_id, muted]() {
            m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
            emit busyChanged();

            try {
                const QString response = watcher->result();
                if(!muted) {
                    m_progressTracker.completeTask(request_id, true);
                }
                emitResponse(request_id, response);
            } catch(const std::exception& exception) {
                if(!muted) {
                    m_progressTracker.completeTask(request_id, false);
                }
                emit errorOccurred(request_id, QString::fromStdString(exception.what()));
            }

            {
                const std::lock_guard lock(m_futuresMutex);
                std::erase_if(m_pendingFutures,
                              [](const QFuture<QString>& future) { return future.isFinished(); });
            }

            watcher->deleteLater();
        });

    watcher->setFuture(future);
    return request_id;
}

QString RequestService::executeOnMainThread(const QString& request_json) {
    auto [request_id, description, injected_json, muted] = prepareRequest(request_json);

    if(!muted) {
        m_progressTracker.beginTask(request_id, description);
    }

    OpenGeoLab::Python::ProgressCallback progress_cb;
    if(!muted) {
        progress_cb = [this, request_id](double progress, std::string_view message) {
            m_progressTracker.updateProgress(
                request_id, progress,
                QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size())));
        };
    }

    try {
        // Main thread GIL note: main.cpp releases GIL before app.exec().
        // runtime_.process() re-acquires the GIL internally, executes Python, then releases.
        // This is intentional for PySide6 invoke_ui — Qt widget creation must happen
        // on the main thread. Keep operations short (e.g. show() a window) to avoid
        // blocking the event loop.
        const auto response = QString::fromStdString(
            m_runtime.process(injected_json.toStdString(), std::move(progress_cb)));
        if(!muted) {
            m_progressTracker.completeTask(request_id, true);
        }
        emitResponse(request_id, response);
    } catch(const std::exception& exception) {
        if(!muted) {
            m_progressTracker.completeTask(request_id, false);
        }
        emit errorOccurred(request_id, QString::fromStdString(exception.what()));
    }

    return request_id;
}

bool RequestService::isBusy() const { return m_pendingCount.load(std::memory_order_relaxed) > 0; }

RequestService::PreparedRequest RequestService::prepareRequest(const QString& json) {
    auto document = QJsonDocument::fromJson(json.toUtf8());
    auto object = document.object();

    const auto module = object.value("module").toString(QStringLiteral("unknown"));
    const auto action = object.value("action").toString(QStringLiteral("unknown"));
    const bool muted = object.value("mute").toBool(false);

    const auto request_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    object.insert(QStringLiteral("requestId"), request_id);

    return {
        request_id,
        QStringLiteral("%1.%2").arg(module, action),
        QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)),
        muted,
    };
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