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
    std::lock_guard lock(m_futuresMutex);
    for (auto& future : m_pendingFutures) {
        future.waitForFinished();
    }
}

bool ProcessService::isBusy() const { return m_pendingCount.load(std::memory_order_relaxed) > 0; }

void ProcessService::submitRequest(const QString& requestJson) {
    const auto document = QJsonDocument::fromJson(requestJson.toUtf8());
    const auto object = document.object();
    const QString module = object.value("module").toString();
    const QString action = object.value("action").toString();

    if(module == QStringLiteral("plugins") && action == QStringLiteral("invoke_ui")) {
        try {
            const auto response =
                QString::fromStdString(m_runtime.process(requestJson.toStdString()));
            const auto responseDocument = QJsonDocument::fromJson(response.toUtf8());
            if(responseDocument.object().value("ok").toBool(false)) {
                emit responseReady(response);
            } else {
                const QString summary =
                    responseDocument.object().value("summary").toString("Unknown error");
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

    auto future = QtConcurrent::run([this, json = requestJson.toStdString()]() -> QString {
        return QString::fromStdString(m_runtime.process(json));
    });

    {
        std::lock_guard lock(m_futuresMutex);
        m_pendingFutures.push_back(future);
    }

    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
        emit busyChanged();

        try {
            const QString response = watcher->result();
            const auto responseDocument = QJsonDocument::fromJson(response.toUtf8());
            if(responseDocument.object().value("ok").toBool(false)) {
                emit responseReady(response);
            } else {
                const QString summary =
                    responseDocument.object().value("summary").toString("Unknown error");
                emit errorOccurred(summary);
            }
        } catch(const std::exception& exception) {
            emit errorOccurred(QString::fromStdString(exception.what()));
        }

        // Remove completed future from tracking list.
        {
            std::lock_guard lock(m_futuresMutex);
            std::erase_if(m_pendingFutures,
                          [](const QFuture<QString>& f) { return f.isFinished(); });
        }

        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

} // namespace OpenGeoLab::App
