#include <doctest/doctest.h>

#include <opengeolab/command/module_dispatcher.hpp>
#include <opengeolab/command/module_service_interface.hpp>

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
        -> CommandResult override {
        return CommandResult{true, "mock ok", nlohmann::json{{"value", 42}}};
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

    const auto response = nlohmann::json::parse(
        dispatcher.dispatch(R"({"module":"nonexistent","action":"test","requestId":"1","payload":{}})"));

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

TEST_CASE("ModuleDispatcher routes to registered mock module") {
    Kangaroo::Util::PluginComponentFactory factory;

    Kangaroo::Util::FactoryRegistration registration{};
    registration.m_interfaceId = "OpenGeoLab.IModuleService";
    registration.m_moduleName = "mock";
    registration.m_lifetime = Kangaroo::Util::ComponentFactoryLifetime::Singleton;
    registration.m_createComponent = [](void*, Kangaroo::Util::ComponentCreateRequest) noexcept -> void* {
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
