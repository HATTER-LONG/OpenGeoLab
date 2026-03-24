#include <doctest/doctest.h>
#include <opengeolab/base/notification_sink.hpp>

#include <string>

namespace OpenGeoLab::Base::Tests {

class RecordingNotificationSink final : public INotificationSink {
public:
    void notify(std::string_view channel, std::string_view payload_json) override {
        lastChannel.assign(channel);
        lastPayload.assign(payload_json);
    }

    std::string lastChannel;
    std::string lastPayload;
};

TEST_CASE("INotificationSink implementations receive channel and payload") {
    RecordingNotificationSink sink;

    sink.notify("progress.update", R"({"value": 1})");

    CHECK(sink.lastChannel == "progress.update");
    CHECK(sink.lastPayload == R"({"value": 1})");
}

} // namespace OpenGeoLab::Base::Tests
