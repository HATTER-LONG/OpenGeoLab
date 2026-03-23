#include <opengeolab/app/process_service.hpp>

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent>

#include <exception>

namespace OpenGeoLab::App {

ProcessService::ProcessService(OpenGeoLab::Python::EmbeddedPythonRuntime& runtime, QObject* parent)
    : QObject(parent), m_runtime(runtime) {}

ProcessService::~ProcessService() {
    // Drain all pending futures so the worker threads never outlive this object.
    const std::lock_guard lock(m_futuresMutex);
    for(auto& future : m_pendingFutures) {
        future.waitForFinished();
    }
}

bool ProcessService::isBusy() const { return m_pendingCount.load(std::memory_order_relaxed) > 0; }

void ProcessService::submitRequest(const QString& request_json) {
    const auto document = QJsonDocument::fromJson(request_json.toUtf8());
    const auto object = document.object();
    const QString module = object.value("module").toString();
    const QString action = object.value("action").toString();

    if(module == QStringLiteral("plugins") && action == QStringLiteral("invoke_ui")) {
        try {
            const auto response =
                QString::fromStdString(m_runtime.process(request_json.toStdString()));
            const auto response_document = QJsonDocument::fromJson(response.toUtf8());
            if(response_document.object().value("ok").toBool(false)) {
                emit responseReady(response);
            } else {
                const QString summary =
                    response_document.object().value("summary").toString("Unknown error");
                emit errorOccurred(summary);
            }
        } catch(const std::exception& exception) {
            emit errorOccurred(QString::fromStdString(exception.what()));
        }
        return;
    }

    m_pendingCount.fetch_add(1, std::memory_order_relaxed);
    emit busyChanged();

    auto* watcher = new QFutureWatcher<QString>(this);

    auto future = QtConcurrent::run([this, json = request_json.toStdString()]() -> QString {
        return QString::fromStdString(m_runtime.process(json));
    });

    {
        const std::lock_guard lock(m_futuresMutex);
        m_pendingFutures.push_back(future);
    }

    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
        emit busyChanged();

        try {
            const QString response = watcher->result();
            const auto response_document = QJsonDocument::fromJson(response.toUtf8());
            if(response_document.object().value("ok").toBool(false)) {
                emit responseReady(response);
            } else {
                const QString summary =
                    response_document.object().value("summary").toString("Unknown error");
                emit errorOccurred(summary);
            }
        } catch(const std::exception& exception) {
            emit errorOccurred(QString::fromStdString(exception.what()));
        }

        // Remove completed future from tracking list.
        {
            const std::lock_guard lock(m_futuresMutex);
            std::erase_if(m_pendingFutures,
                          [](const QFuture<QString>& f) { return f.isFinished(); });
        }

        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

} // namespace OpenGeoLab::App
