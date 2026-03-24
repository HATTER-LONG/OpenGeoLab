#include <doctest/doctest.h>
#include <opengeolab/base/notification_sink.hpp>

#include <string>

namespace OpenGeoLab::Base::Tests {

class RecordingNotificationSink final : public INotificationSink {
public:
    void notify(std::string_view channel, std::string_view payload_json) override {
        last_channel.assign(channel);
        last_payload.assign(payload_json);
    }

    std::string last_channel;
    std::string last_payload;
};

TEST_CASE("INotificationSink implementations receive channel and payload") {
    RecordingNotificationSink sink;

    sink.notify("progress.update", R"({"value": 1})");

    CHECK(sink.last_channel == "progress.update");
    CHECK(sink.last_payload == R"({"value": 1})");
}

} // namespace OpenGeoLab::Base::Tests
