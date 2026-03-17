#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <ogl/command/RequestProtocol.hpp>

TEST_CASE("request protocol normalizes missing param to object", "[command][protocol]") {
    const auto normalized =
        OGL::Command::normalizeRequestParamObject({{"module", "geometry"}, {"action", "createBox"}});

    REQUIRE(normalized.is_object());
    REQUIRE(normalized.at("param").is_object());
    CHECK(normalized.at("module") == "geometry");
    CHECK(normalized.at("action") == "createBox");
}

TEST_CASE("request protocol preserves extra top-level keys", "[command][protocol]") {
    const auto normalized =
        OGL::Command::normalizeRequestParamObject({{"module", "geometry"},
                                                   {"action", "createBox"},
                                                   {"source", "unit-test"},
                                                   {"requestId", 7}});

    CHECK(normalized.at("source") == "unit-test");
    CHECK(normalized.at("requestId") == 7);
    REQUIRE(normalized.at("param").is_object());
}

TEST_CASE("request protocol rejects invalid envelopes", "[command][protocol]") {
    SECTION("non-object request roots are rejected") {
        REQUIRE_THROWS_WITH(OGL::Command::normalizeRequestParamObject(nlohmann::json::array()),
                            Catch::Matchers::ContainsSubstring("Request payload must be a JSON object."));
    }

    SECTION("missing module is rejected") {
        REQUIRE_THROWS_WITH(OGL::Command::parseCommandRequest({{"action", "createBox"}}),
                            Catch::Matchers::ContainsSubstring("Request module cannot be empty."));
    }

    SECTION("missing action is rejected") {
        REQUIRE_THROWS_WITH(OGL::Command::parseCommandRequest({{"module", "geometry"}}),
                            Catch::Matchers::ContainsSubstring("Request action cannot be empty."));
    }

    SECTION("empty module is rejected") {
        REQUIRE_THROWS_WITH(
            OGL::Command::parseCommandRequest({{"module", ""}, {"action", "createBox"}}),
            Catch::Matchers::ContainsSubstring("Request module cannot be empty."));
    }

    SECTION("empty action is rejected") {
        REQUIRE_THROWS_WITH(
            OGL::Command::parseCommandRequest({{"module", "geometry"}, {"action", ""}}),
            Catch::Matchers::ContainsSubstring("Request action cannot be empty."));
    }

    SECTION("scalar param is rejected") {
        REQUIRE_THROWS_WITH(
            OGL::Command::parseCommandRequest(
                {{"module", "geometry"}, {"action", "createBox"}, {"param", 1}}),
            Catch::Matchers::ContainsSubstring("Request param must be a JSON object."));
    }
}

TEST_CASE("request protocol parses normalized command request", "[command][protocol]") {
    const auto request = OGL::Command::parseCommandRequest(
        {{"module", "geometry"}, {"action", "createBox"}, {"param", nullptr}});

    CHECK(request.module == "geometry");
    CHECK(request.action == "createBox");
    REQUIRE(request.param.is_object());
    CHECK(request.param.empty());
}
