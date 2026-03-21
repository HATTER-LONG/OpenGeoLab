#include <doctest/doctest.h>

#include <opengeolab/command/request_recorder.hpp>

#include <string>
#include <vector>

namespace OpenGeoLab::Command {
namespace {

TEST_CASE("RequestRecorder start and stop toggle recording state") {
    RequestRecorder recorder;

    CHECK_FALSE(recorder.isRecording());

    recorder.start();

    CHECK(recorder.isRecording());

    recorder.stop();

    CHECK_FALSE(recorder.isRecording());
}

TEST_CASE("RequestRecorder records requests only while recording") {
    RequestRecorder recorder;

    recorder.record(R"({"requestId":"before"})");

    CHECK(recorder.get().empty());

    recorder.start();
    recorder.record(R"({"requestId":"during"})");
    recorder.stop();
    recorder.record(R"({"requestId":"after"})");

    const std::vector<std::string> expected{R"({"requestId":"during"})"};
    CHECK(recorder.get() == expected);
}

TEST_CASE("RequestRecorder get returns recorded requests") {
    RequestRecorder recorder;

    recorder.start();
    recorder.record(R"({"requestId":"1"})");
    recorder.record(R"({"requestId":"2"})");

    const std::vector<std::string> expected{
        R"({"requestId":"1"})",
        R"({"requestId":"2"})",
    };
    CHECK(recorder.get() == expected);
}

TEST_CASE("RequestRecorder clear empties buffer without changing recording state") {
    RequestRecorder recorder;

    recorder.start();
    recorder.record(R"({"requestId":"1"})");

    recorder.clear();

    CHECK(recorder.isRecording());
    CHECK(recorder.get().empty());

    recorder.record(R"({"requestId":"2"})");

    const std::vector<std::string> expected{R"({"requestId":"2"})"};
    CHECK(recorder.get() == expected);
}

TEST_CASE("RequestRecorder repeated start preserves existing buffer") {
    RequestRecorder recorder;

    recorder.start();
    recorder.record(R"({"requestId":"1"})");

    recorder.start();
    recorder.record(R"({"requestId":"2"})");

    const std::vector<std::string> expected{
        R"({"requestId":"1"})",
        R"({"requestId":"2"})",
    };
    CHECK(recorder.get() == expected);
}

TEST_CASE("RequestRecorder ignores records after stop") {
    RequestRecorder recorder;

    recorder.start();
    recorder.record(R"({"requestId":"1"})");
    recorder.stop();
    recorder.record(R"({"requestId":"2"})");

    const std::vector<std::string> expected{R"({"requestId":"1"})"};
    CHECK(recorder.get() == expected);
}

} // namespace
} // namespace OpenGeoLab::Command
