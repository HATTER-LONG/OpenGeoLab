/**
 * @file io_module_test.cpp
 * @brief Unit tests for IOModule and ReadBrepAction
 */

#include <opengeolab/io/io_module.hpp>
#include <opengeolab/io/read_brep_action.hpp>

#include <doctest/doctest.h>

TEST_CASE("IOModule describe returns module info with actions") {
    OpenGeoLab::IO::IOModule mod;
    auto desc = mod.describe();
    CHECK(desc["name"] == "io");
    CHECK(desc.contains("description"));
    CHECK(desc["actions"].is_array());
    CHECK(desc["actions"].size() == 1);
    CHECK(desc["actions"][0]["name"] == "read_brep");
}

TEST_CASE("IOModule dispatches read_brep action") {
    OpenGeoLab::IO::IOModule mod;
    nlohmann::json request = {
        {"module", "io"}, {"action", "read_brep"}, {"param", {{"path", "test.brep"}}}};
    auto result = mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["status"] == "ok");
    CHECK(result["action"] == "read_brep");
    CHECK(result["path"] == "test.brep");
}

TEST_CASE("IOModule throws on missing action field") {
    OpenGeoLab::IO::IOModule mod;
    nlohmann::json request = {{"module", "io"}, {"param", {{"path", "test.brep"}}}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("IOModule throws on unknown action") {
    OpenGeoLab::IO::IOModule mod;
    nlohmann::json request = {{"module", "io"}, {"action", "unknown_action"}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("ReadBrepAction describe returns action metadata") {
    OpenGeoLab::IO::ReadBrepAction action;
    auto desc = action.describe();
    CHECK(desc["name"] == "read_brep");
    CHECK(desc.contains("description"));
    CHECK(desc.contains("params"));
    CHECK(desc["params"].contains("path"));
}

TEST_CASE("ReadBrepAction throws on missing path") {
    OpenGeoLab::IO::ReadBrepAction action;
    nlohmann::json param = {};
    CHECK_THROWS_AS((void)action.execute(param, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}
