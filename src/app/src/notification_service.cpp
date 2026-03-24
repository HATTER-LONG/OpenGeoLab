#include <opengeolab/app/notification_service.hpp>

#include <QMetaObject>

#include <utility>

namespace OpenGeoLab::App {

NotificationService::NotificationService(QObject* parent) : QObject(parent) {}

void NotificationService::notify(std::string_view channel, std::string_view payload_json) {
    const std::string channel_string(channel);
    const std::string payload_string(payload_json);

    BufferState* matched_state = nullptr;
    std::string matched_prefix;
    int matched_interval_ms = 16;

    {
        const std::lock_guard lock(m_configMutex);
        for(const auto& buffer_config : m_bufferConfigs) {
            if(!channel_string.starts_with(buffer_config.prefix)) {
                continue;
            }

            matched_prefix = buffer_config.prefix;
            matched_interval_ms = buffer_config.intervalMs;

            const auto state_iterator = m_bufferStates.find(matched_prefix);
            if(state_iterator != m_bufferStates.end()) {
                matched_state = state_iterator->second.get();
            }
            break;
        }
    }

    if(matched_state != nullptr) {
        bool should_schedule_timer = false;
        {
            const std::lock_guard lock(matched_state->mutex);
            matched_state->pending.push_back(payload_string);
            if(!matched_state->timerScheduled) {
                matched_state->timerScheduled = true;
                should_schedule_timer = true;
            }
        }

        if(should_schedule_timer) {
            QMetaObject::invokeMethod(
                this,
                [this, prefix = matched_prefix, interval_ms = matched_interval_ms]() {
                    QTimer::singleShot(interval_ms, this,
                                       [this, prefix]() { flushBuffer(prefix); });
                },
                Qt::QueuedConnection);
        }
        return;
    }

    auto qt_channel = QString::fromUtf8(channel.data(), static_cast<qsizetype>(channel.size()));
    auto qt_payload =
        QString::fromUtf8(payload_json.data(), static_cast<qsizetype>(payload_json.size()));
    QMetaObject::invokeMethod(
        this,
        [this, qt_channel = std::move(qt_channel), qt_payload = std::move(qt_payload)]() {
            emit notificationReceived(qt_channel, qt_payload);
        },
        Qt::QueuedConnection);
}

void NotificationService::enableBuffering(const QString& channel_prefix, int interval_ms) {
    const std::lock_guard lock(m_configMutex);

    m_bufferConfigs.push_back(BufferConfig{channel_prefix.toStdString(), interval_ms});
    m_bufferStates.try_emplace(m_bufferConfigs.back().prefix, std::make_unique<BufferState>());
}

void NotificationService::flushBuffer(const std::string& prefix) {
    BufferState* buffer_state = nullptr;
    {
        const std::lock_guard lock(m_configMutex);
        const auto state_iterator = m_bufferStates.find(prefix);
        if(state_iterator == m_bufferStates.end()) {
            return;
        }

        buffer_state = state_iterator->second.get();
    }

    std::vector<std::string> payloads;
    {
        const std::lock_guard lock(buffer_state->mutex);
        payloads.swap(buffer_state->pending);
        buffer_state->timerScheduled = false;
    }

    if(payloads.empty()) {
        return;
    }

    QString json_array = QStringLiteral("[");
    for(std::size_t index = 0; index < payloads.size(); ++index) {
        if(index > 0) {
            json_array += QStringLiteral(",");
        }
        json_array += QString::fromStdString(payloads[index]);
    }
    json_array += QStringLiteral("]");

    emit notificationReceived(QString::fromStdString(prefix), json_array);
}

} // namespace OpenGeoLab::App
