/**
 * @file notification_sink.hpp
 * @brief Abstract notification sink interface for cross-layer messaging.
 */
#pragma once

#include <opengeolab/base/base_export.hpp>

#include <string_view>

namespace OpenGeoLab::Base {

/**
 * @brief Abstract interface for components that can receive notifications.
 *
 * Libs emit notifications through Kangaroo Signal; the app layer connects
 * them to a concrete NotificationService that implements this interface.
 */
class OPENGEOLAB_BASE_EXPORT INotificationSink {
public:
    virtual ~INotificationSink() = default;

    /**
     * @brief Deliver a notification.
     * @param channel Logical channel name (e.g. "llm.token", "progress.update").
     * @param payload_json JSON-encoded payload string.
     * @note Thread-safe: implementations must handle calls from any thread.
     */
    virtual void notify(std::string_view channel, std::string_view payload_json) = 0;
};

} // namespace OpenGeoLab::Base
