/**
 * @file notification_registry.hpp
 * @brief Global registry for the application notification sink.
 */
#pragma once

#include <opengeolab/base/base_export.hpp>

namespace OpenGeoLab::Base {

class INotificationSink;

/**
 * @brief Static registry for the application-level notification sink.
 *
 * Set once at application startup via setSink(). Libraries that need to emit
 * notifications read the pointer with sink(). Returns nullptr if no sink has
 * been registered. Not thread-safe for registration — must be called from the
 * main thread before any worker threads start.
 */
class OPENGEOLAB_BASE_EXPORT NotificationRegistry {
public:
    NotificationRegistry() = delete;

    /** @brief Register the application notification sink. Call once at startup. */
    static void setSink(INotificationSink* sink);

    /** @brief Retrieve the registered sink, or nullptr if none. */
    [[nodiscard]] static INotificationSink* sink();

private:
    static inline INotificationSink* sSink = nullptr;
};

} // namespace OpenGeoLab::Base
