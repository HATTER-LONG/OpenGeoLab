#include <catch2/catch_test_macros.hpp>

#include <ogl/command/CommandService.hpp>

#include <string>
#include <vector>

TEST_CASE("command request serializes the canonical envelope", "[command][unit]") {
    const OGL::Command::CommandRequest request{
        .module = "geometry",
        .action = "inspectModel",
        .param = {{"modelName", "UnitModel"}, {"bodyCount", 2}},
    };

    const auto request_json = request.toJson();
    CHECK(request_json.at("module") == "geometry");
    CHECK(request_json.at("action") == "inspectModel");
    CHECK(request_json.at("param").at("modelName") == "UnitModel");
    CHECK(request_json.at("param").at("bodyCount") == 2);
}

TEST_CASE("command service exports bridge-based Python replay script", "[command][unit]") {
    const std::vector<OGL::Command::CommandRequest> requests{
        {.module = "geometry",
         .action = "inspectModel",
         .param = {{"modelName", "ScriptModel"}, {"bodyCount", 1}}},
    };

    const std::string script = OGL::Command::CommandService::exportPythonScript(requests);
    CHECK(script.find("OpenGeoLabPythonBridge") != std::string::npos);
    CHECK(script.find("bridge.process(request_1)") != std::string::npos);
    CHECK(script.find("\"action\": \"inspectModel\"") != std::string::npos);
}
