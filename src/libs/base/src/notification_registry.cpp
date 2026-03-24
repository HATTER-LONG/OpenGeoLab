#include <opengeolab/base/notification_registry.hpp>

namespace OpenGeoLab::Base {

void NotificationRegistry::setSink(INotificationSink* sink) { sSink = sink; }

INotificationSink* NotificationRegistry::sink() { return sSink; }

} // namespace OpenGeoLab::Base
