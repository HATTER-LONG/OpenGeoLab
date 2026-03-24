/// @file notification_service.hpp
/// @brief Qt-based notification service bridging cross-thread notifications to QML.
#pragma once

#include <opengeolab/base/notification_sink.hpp>

#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::App {

/// @brief Concrete notification service that bridges INotificationSink to Qt signals.
///
/// Thread-safe: notify() can be called from any thread. The notificationReceived
/// signal is always emitted on the main thread via QueuedConnection.
/// Optionally buffers high-frequency channels (e.g. LLM token streams) to reduce
/// signal emission overhead.
class NotificationService : public QObject, public OpenGeoLab::Base::INotificationSink {
    Q_OBJECT

public:
    explicit NotificationService(QObject* parent = nullptr);

    /// @brief INotificationSink implementation. Thread-safe.
    /// Delivers notification to the main thread via QueuedConnection.
    /// If the channel matches a buffered prefix, accumulates and flushes periodically.
    void notify(std::string_view channel, std::string_view payload_json) override;

    /// @brief Enable buffered delivery for a channel prefix.
    /// Messages matching the prefix are accumulated for interval_ms milliseconds,
    /// then delivered as a JSON array in a single notificationReceived emission.
    /// @param channel_prefix Dot-separated prefix (e.g. "llm.stream").
    /// @param interval_ms Buffering interval in milliseconds (default 16ms ≈ 60fps).
    void enableBuffering(const QString& channel_prefix, int interval_ms = 16);

signals:
    /// @brief Emitted on the main thread for every notification (or buffered batch).
    /// @param channel The notification channel name.
    /// @param payload JSON payload string. For buffered channels, a JSON array.
    void notificationReceived(const QString& channel, const QString& payload);

private:
    struct BufferConfig {
        std::string prefix;
        int interval_ms = 16;
    };

    struct BufferState {
        std::mutex mutex;
        std::vector<std::string> pending;
        bool timer_scheduled = false;
    };

    void flushBuffer(const std::string& prefix);

    std::vector<BufferConfig> buffer_configs_;
    /// Stored as unique_ptr so that map rehashing does not invalidate BufferState pointers.
    std::unordered_map<std::string, std::unique_ptr<BufferState>> buffer_states_;
    std::mutex config_mutex_;
};

} // namespace OpenGeoLab::App
