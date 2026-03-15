#include <catch2/catch_test_macros.hpp>

#include <ogl/command/CommandService.hpp>

#include <nlohmann/json.hpp>

#include <string>

TEST_CASE("command recorder records replays exports and clears", "[command][smoke]") {
    OGL::Command::CommandRecorder recorder;

    const auto response =
        recorder.execute({.module = "selection",
                          .action = "pickEntity",
                          .param = nlohmann::json{{"modelName", "CommandSmokeModel"},
                                                  {"bodyCount", 3},
                                                  {"viewportWidth", 800},
                                                  {"viewportHeight", 600},
                                                  {"screenX", 128},
                                                  {"screenY", 96},
                                                  {"source", "test"}}});

    REQUIRE(response.success);
    REQUIRE(recorder.recordedCount() == 1);

    const auto replay_report = recorder.replayAll();
    CHECK(replay_report.replayedCount == 1);
    CHECK(replay_report.successCount == 1);

    const std::string script = recorder.exportedPythonScript();
    CHECK(script.find("OpenGeoLabPythonBridge") != std::string::npos);
    CHECK(script.find("selection") != std::string::npos);

    recorder.clear();
    CHECK(recorder.recordedCount() == 0);
}
