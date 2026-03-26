#include "opengeolab/app/request_service.h"

#include <opengeolab/python_embed/embedded_python_runtime.hpp>

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QtConcurrent>

#include <exception>

namespace OpenGeoLab::App {

RequestService::RequestService(OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                               QObject* parent)
    : QObject(parent), m_runtime(runtime) {}

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

    auto* watcher = new QFutureWatcher<QString>(this);

    auto future = QtConcurrent::run([this, json = injected_json.toStdString()]() -> QString {
        return QString::fromStdString(m_runtime.process(json, nullptr));
    });

    {
        const std::lock_guard lock(m_futuresMutex);
        m_pendingFutures.push_back(future);
    }

    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, request_id]() {
        m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
        emit busyChanged();

        try {
            const QString response = watcher->result();
            emitResponse(request_id, response);
        } catch(const std::exception& exception) {
            emit errorOccurred(request_id, QString::fromStdString(exception.what()));
        }

        {
            const std::lock_guard lock(m_futuresMutex);
            std::erase_if(m_pendingFutures,
                          [](const QFuture<QString>& f) { return f.isFinished(); });
        }

        watcher->deleteLater();
    });

    watcher->setFuture(future);
    return request_id;
}

QString RequestService::executeOnMainThread(const QString& request_json) {
    auto [request_id, description, injected_json, muted] = prepareRequest(request_json);

    try {
        // main.cpp releases GIL before app.exec(). runtime.process() re-acquires
        // GIL internally. This allows PySide6 launch_ui() to create Qt widgets on
        // the main thread safely.
        const auto response =
            QString::fromStdString(m_runtime.process(injected_json.toStdString(), nullptr));
        emitResponse(request_id, response);
    } catch(const std::exception& exception) {
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
