#include "opengeolab/app/request_service.h"

#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/core/logger.hpp>
#include <opengeolab/core/progress_callback.hpp>
#include <opengeolab/python_embed/embedded_python_runtime.hpp>

#include <kangaroo/util/stopwatch.hpp>
#include <nlohmann/json.hpp>

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent>

#include <exception>
#include <optional>

namespace OpenGeoLab::App {

RequestService::RequestService(OpenGeoLab::Command::CommandDispatcher& dispatcher,
                               OpenGeoLab::PythonEmbed::EmbeddedPythonRuntime& runtime,
                               QObject* parent)
    : QObject(parent), m_dispatcher(dispatcher), m_runtime(runtime) {}

RequestService::~RequestService() {
    const std::lock_guard lock(m_futuresMutex);
    for(auto& future : m_pendingFutures) {
        future.waitForFinished();
    }
}

void RequestService::submitAsync(const QString& request_json) {
    auto [description, process_json, module_name, muted] = prepareRequest(request_json);
    if(!muted) {
        LOG_INFO("RequestService: submitting async [{}]", description.toStdString());
    }

    emit requestSent(description, request_json, muted);

    m_pendingCount.fetch_add(1, std::memory_order_relaxed);
    emit busyChanged();

    auto* watcher = new QFutureWatcher<QString>(this);

    Core::ProgressCallback progress_cb = [this](double progress,
                                                const std::string& message) -> bool {
        QMetaObject::invokeMethod(
            this,
            [this, progress, msg = QString::fromStdString(message)]() {
                emit progressUpdated(progress, msg);
            },
            Qt::QueuedConnection);
        return true;
    };

    const bool use_cpp_path = m_dispatcher.hasModule(module_name);

    auto future = QtConcurrent::run([this, json = process_json.toStdString(),
                                     cb = std::move(progress_cb), muted,
                                     desc = description.toStdString(), use_cpp_path]() -> QString {
        std::optional<Kangaroo::Util::Stopwatch> sw;
        if(!muted) {
            sw.emplace(desc, Core::getLoggerShared());
        }
        if(use_cpp_path) {
            nlohmann::json request;
            try {
                request = nlohmann::json::parse(json);
            } catch(const nlohmann::json::parse_error& e) {
                nlohmann::json err = {{"ok", false},
                                      {"summary", "Invalid JSON in request"},
                                      {"errors", nlohmann::json::array({std::string(e.what())})}};
                return QString::fromStdString(err.dump());
            }
            auto result = m_dispatcher.dispatch(request, cb);
            return QString::fromStdString(result.dump());
        }
        return QString::fromStdString(m_runtime.process(json, cb));
    });

    {
        const std::lock_guard lock(m_futuresMutex);
        m_pendingFutures.push_back(future);
    }

    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, muted]() {
        m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
        emit busyChanged();

        try {
            const QString response = watcher->result();
            emitResponse(response, muted);
        } catch(const std::exception& exception) {
            LOG_ERROR("RequestService: async request threw exception: {}{}", exception.what(),
                      muted ? " (muted)" : "");
            emit errorOccurred(QString::fromStdString(exception.what()), muted);
        }

        {
            const std::lock_guard lock(m_futuresMutex);
            std::erase_if(m_pendingFutures,
                          [](const QFuture<QString>& f) { return f.isFinished(); });
        }

        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void RequestService::executeOnMainThread(const QString& request_json) {
    auto [description, process_json, module_name, muted] = prepareRequest(request_json);

    if(!muted) {
        LOG_INFO("RequestService: executing on main thread [{}]", description.toStdString());
    }

    emit requestSent(description, request_json, muted);

    try {
        std::optional<Kangaroo::Util::Stopwatch> sw;
        if(!muted) {
            sw.emplace(description.toStdString(), Core::getLoggerShared());
        }

        if(m_dispatcher.hasModule(module_name)) {
            auto request = nlohmann::json::parse(process_json.toStdString());
            auto result = m_dispatcher.dispatch(request, nullptr);
            emitResponse(QString::fromStdString(result.dump()), muted);
        } else {
            // Python path: process() re-acquires GIL internally.
            const auto response =
                QString::fromStdString(m_runtime.process(process_json.toStdString(), nullptr));
            emitResponse(response, muted);
        }
    } catch(const std::exception& exception) {
        LOG_ERROR("RequestService: main-thread request threw exception: {}{}", exception.what(),
                  muted ? " (muted)" : "");
        emit errorOccurred(QString::fromStdString(exception.what()), muted);
    }
}

bool RequestService::isBusy() const { return m_pendingCount.load(std::memory_order_relaxed) > 0; }

RequestService::PreparedRequest RequestService::prepareRequest(const QString& json) {
    auto document = QJsonDocument::fromJson(json.toUtf8());
    auto object = document.object();

    const auto module = object.value("module").toString(QStringLiteral("unknown"));
    const auto action = object.value("action").toString(QStringLiteral("unknown"));
    const bool muted = object.value("mute").toBool(false);

    return {
        QStringLiteral("%1.%2").arg(module, action),
        json,
        module.toStdString(),
        muted,
    };
}

void RequestService::emitResponse(const QString& response, bool muted) {
    const auto document = QJsonDocument::fromJson(response.toUtf8());
    const auto object = document.object();
    if(object.value("ok").toBool(false)) {
        emit responseReady(response, muted);
    } else {
        const auto summary = object.value("summary").toString(QStringLiteral("Unknown error"));
        emit errorOccurred(summary, muted);
    }
}

} // namespace OpenGeoLab::App
