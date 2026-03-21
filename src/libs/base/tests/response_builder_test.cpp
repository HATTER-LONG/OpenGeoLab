#include <doctest/doctest.h>

#include <opengeolab/base/protocol_constants.hpp>
#include <opengeolab/base/response_builder.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace Base = OpenGeoLab::Base;

namespace {

TEST_CASE("makeResponse with module includes module field") {
    const auto response = nlohmann::json::parse(Base::makeResponse(
        "req-1", "scene", "snapshot", true, "ok", {{"value", 1}}, nlohmann::json::array()));

    CHECK(response.contains("protocolVersion"));
    CHECK(response.contains("requestId"));
    CHECK(response.contains("ok"));
    CHECK(response.contains("module"));
    CHECK(response.contains("action"));
    CHECK(response.contains("summary"));
    CHECK(response.contains("result"));
    CHECK(response.contains("errors"));
}

TEST_CASE("makeResponse without module omits module field") {
    const auto response = nlohmann::json::parse(Base::makeResponse(
        "req-2", "snapshot", true, "ok", {{"value", 2}}, nlohmann::json::array()));

    CHECK(response.contains("protocolVersion"));
    CHECK(response.contains("requestId"));
    CHECK(response.contains("ok"));
    CHECK_FALSE(response.contains("module"));
    CHECK(response.contains("action"));
    CHECK(response.contains("summary"));
    CHECK(response.contains("result"));
    CHECK(response.contains("errors"));
}

TEST_CASE("makeErrorResponse with module uses structured error items") {
    const auto response =
        nlohmann::json::parse(Base::makeErrorResponse("req-3", "scene", "snapshot", "boom"));

    REQUIRE(response.at("ok") == false);
    REQUIRE(response.at("errors").is_array());
    REQUIRE(response.at("errors").size() == 1);
    CHECK(response.at("errors").front().is_object());
    CHECK(response.at("errors").front().contains("message"));
    CHECK(response.at("errors").front().at("message") == "boom");
}

TEST_CASE("makeErrorResponse without module uses plain string array") {
    const auto response =
        nlohmann::json::parse(Base::makeErrorResponse("req-4", "snapshot", "boom"));

    REQUIRE(response.at("ok") == false);
    REQUIRE(response.at("errors").is_array());
    REQUIRE(response.at("errors").size() == 1);
    CHECK(response.at("errors").front().is_string());
    CHECK(response.at("errors").front() == "boom");
}

TEST_CASE("makeErrorItem returns structured object") {
    CHECK(Base::makeErrorItem("test msg") == nlohmann::json{{"message", "test msg"}});
}

TEST_CASE("protocol version is 1.0") { CHECK(Base::kProtocolVersion == "1.0"); }

} // namespace
