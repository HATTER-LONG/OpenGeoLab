#include <doctest/doctest.h>

#include <opengeolab/base/module_service_interface.hpp>
#include <opengeolab/command/module_dispatcher.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Command {
namespace {

class MockModuleService final : public IModuleService {
public:
    [[nodiscard]] auto moduleName() const noexcept -> std::string_view override { return "mock"; }

    [[nodiscard]] auto dispatch(std::string_view /*action*/, const nlohmann::json& /*payload*/)
        -> Base::CommandResult override {
        return Base::CommandResult{true, "mock ok", nlohmann::json{{"value", 42}}};
    }

    [[nodiscard]] auto supportedActions() const -> std::vector<std::string> override {
        return {"test"};
    }
};

TEST_CASE("ModuleDispatcher returns error for malformed JSON") {
    Kangaroo::Util::PluginComponentFactory factory;
    ModuleDispatcher dispatcher(factory);

    const auto response = nlohmann::json::parse(dispatcher.dispatch("not json"));

    CHECK(response.at("protocolVersion") == "1.0");
    CHECK(response.at("requestId").is_null());
    CHECK(response.at("module").is_null());
    CHECK(response.at("action").is_null());
    CHECK(response.at("ok") == false);
    REQUIRE(response.at("errors").is_array());
    CHECK_FALSE(response.at("errors").empty());
    CHECK(response.at("summary").get_ref<const std::string&>().empty() == false);
}

TEST_CASE("ModuleDispatcher returns error for unknown module") {
    Kangaroo::Util::PluginComponentFactory factory;
    ModuleDispatcher dispatcher(factory);

    const auto response = nlohmann::json::parse(dispatcher.dispatch(
        R"({"module":"nonexistent","action":"test","requestId":"1","payload":{}})"));

    CHECK(response.at("protocolVersion") == "1.0");
    CHECK(response.at("requestId") == "1");
    CHECK(response.at("module") == "nonexistent");
    CHECK(response.at("action") == "test");
    CHECK(response.at("ok") == false);
    REQUIRE(response.at("errors").is_array());
    REQUIRE_FALSE(response.at("errors").empty());
    CHECK(response.at("summary") == "Unknown module: nonexistent");
    CHECK(response.at("errors").at(0).at("message") == "Unknown module: nonexistent");
}

TEST_CASE("ModuleDispatcher records requests when dispatch runs with recording enabled") {
    Kangaroo::Util::PluginComponentFactory factory;
    ModuleDispatcher dispatcher(factory);

    dispatcher.startRecording();
    static_cast<void>(dispatcher.dispatch(
        R"({"module":"nonexistent","action":"test","requestId":"1","payload":{}})"));

    REQUIRE(dispatcher.isRecording());
    const auto& recorded_requests = dispatcher.getRecordedRequests();
    REQUIRE(recorded_requests.size() == 1);
    CHECK(recorded_requests.front() ==
          R"({"module":"nonexistent","action":"test","requestId":"1","payload":{}})");
}

TEST_CASE("ModuleDispatcher does not record requests when dispatch runs with recording disabled") {
    Kangaroo::Util::PluginComponentFactory factory;
    ModuleDispatcher dispatcher(factory);

    static_cast<void>(dispatcher.dispatch(
        R"({"module":"nonexistent","action":"test","requestId":"1","payload":{}})"));

    CHECK_FALSE(dispatcher.isRecording());
    CHECK(dispatcher.getRecordedRequests().empty());
}

TEST_CASE("ModuleDispatcher does not record requests when JSON parsing fails") {
    Kangaroo::Util::PluginComponentFactory factory;
    ModuleDispatcher dispatcher(factory);

    dispatcher.startRecording();
    static_cast<void>(dispatcher.dispatch("not json"));

    REQUIRE(dispatcher.isRecording());
    CHECK(dispatcher.getRecordedRequests().empty());
}

TEST_CASE("ModuleDispatcher does not record requests when module or action validation fails") {
    Kangaroo::Util::PluginComponentFactory factory;
    ModuleDispatcher dispatcher(factory);

    dispatcher.startRecording();
    static_cast<void>(
        dispatcher.dispatch(R"({"module":123,"action":"test","requestId":"1","payload":{}})"));
    static_cast<void>(
        dispatcher.dispatch(R"({"module":"mock","action":123,"requestId":"2","payload":{}})"));

    REQUIRE(dispatcher.isRecording());
    CHECK(dispatcher.getRecordedRequests().empty());
}

TEST_CASE("ModuleDispatcher routes to registered mock module") {
    Kangaroo::Util::PluginComponentFactory factory;

    Kangaroo::Util::FactoryRegistration registration{};
    registration.m_interfaceId = "OpenGeoLab.IModuleService";
    registration.m_moduleName = "mock";
    registration.m_lifetime = Kangaroo::Util::ComponentFactoryLifetime::Singleton;
    registration.m_createComponent = [](void*,
                                        Kangaroo::Util::ComponentCreateRequest) noexcept -> void* {
        return new MockModuleService();
    };
    registration.m_destroyComponent = [](void*, void* object) noexcept {
        delete static_cast<MockModuleService*>(object);
    };

    factory.registerFactory(registration);
    ModuleDispatcher dispatcher(factory);

    const auto response = nlohmann::json::parse(
        dispatcher.dispatch(R"({"module":"mock","action":"test","requestId":"t1","payload":{}})"));

    REQUIRE(response.at("ok") == true);
    CHECK(response.at("protocolVersion") == "1.0");
    CHECK(response.at("requestId") == "t1");
    CHECK(response.at("module") == "mock");
    CHECK(response.at("action") == "test");
    CHECK(response.at("summary") == "mock ok");
    REQUIRE(response.at("result").is_object());
    CHECK(response.at("result").at("value") == 42);
    REQUIRE(response.at("errors").is_array());
    CHECK(response.at("errors").empty());

    REQUIRE(factory.unregisterModule("mock") == 1);
}

} // namespace
} // namespace OpenGeoLab::Command
