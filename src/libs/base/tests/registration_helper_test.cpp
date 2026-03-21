#include <doctest/doctest.h>

#include <opengeolab/base/action_interface.hpp>
#include <opengeolab/base/module_service_interface.hpp>
#include <opengeolab/base/registration_helper.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Base {
namespace {

class TestModule final : public IModuleService {
public:
    explicit TestModule(Kangaroo::Util::PluginComponentFactory& factory) : m_factory(&factory) {}

    [[nodiscard]] auto moduleName() const noexcept -> std::string_view override { return "test"; }

    [[nodiscard]] auto dispatch(std::string_view, const nlohmann::json&) -> CommandResult override {
        return CommandResult{.ok = true, .summary = "ok", .result = nlohmann::json::object()};
    }

    [[nodiscard]] auto supportedActions() const -> std::vector<std::string> override {
        return {"a"};
    }

    [[nodiscard]] auto factoryAddress() const noexcept -> const void* { return m_factory; }

private:
    Kangaroo::Util::PluginComponentFactory* m_factory;
};

class DefaultAction final : public IAction {
public:
    DefaultAction() = default;

    [[nodiscard]] auto actionName() const noexcept -> std::string_view override {
        return "test.default";
    }

    [[nodiscard]] auto execute(const nlohmann::json&) -> CommandResult override {
        return CommandResult{.ok = true, .summary = "default", .result = nlohmann::json::object()};
    }
};

class InjectedAction final : public IAction {
public:
    explicit InjectedAction(std::shared_ptr<int> value) : m_value(std::move(value)) {}

    [[nodiscard]] auto actionName() const noexcept -> std::string_view override {
        return "test.injected";
    }

    [[nodiscard]] auto execute(const nlohmann::json&) -> CommandResult override {
        return CommandResult{.ok = true, .summary = "injected", .result = {{"value", *m_value}}};
    }

private:
    std::shared_ptr<int> m_value;
};

TEST_CASE("registerModule registers singleton module services") {
    Kangaroo::Util::PluginComponentFactory factory;

    registerModule<TestModule>(factory, "test.module");

    auto first = factory.getSharedInstance<IModuleService>("test.module");
    auto second = factory.getSharedInstance<IModuleService>("test.module");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    CHECK(first.get() == second.get());
    CHECK(first->moduleName() == "test");
    CHECK(static_cast<const TestModule&>(*first).factoryAddress() == &factory);

    REQUIRE(factory.unregisterModule("test.module") == 1);
}

TEST_CASE("registerAction registers transient default-constructed actions") {
    Kangaroo::Util::PluginComponentFactory factory;

    registerAction<DefaultAction>(factory, "test.default");

    auto first = factory.create<IAction>("test.default");
    auto second = factory.create<IAction>("test.default");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    CHECK(first.get() != second.get());
    CHECK(first->actionName() == "test.default");
    CHECK(second->actionName() == "test.default");

    REQUIRE(factory.unregisterModule("test.default") == 1);
}

TEST_CASE("registerAction accepts raw create and destroy callbacks") {
    Kangaroo::Util::PluginComponentFactory factory;

    registerAction<InjectedAction>(
        factory, "test.injected",
        [](void*, Kangaroo::Util::ComponentCreateRequest request) noexcept -> void* {
            if(request.m_data == nullptr) {
                return nullptr;
            }

            auto value = *static_cast<const std::shared_ptr<int>*>(request.m_data);
            return new InjectedAction(std::move(value));
        },
        [](void*, void* object) noexcept { delete static_cast<InjectedAction*>(object); });

    auto value = std::make_shared<int>(42);
    auto action = factory.create<IAction>("test.injected",
                                          Kangaroo::Util::ComponentCreateRequest::from(value));

    REQUIRE(action != nullptr);
    CHECK(action->actionName() == "test.injected");
    CHECK(action->execute(nlohmann::json::object()).result.at("value") == 42);

    REQUIRE(factory.unregisterModule("test.injected") == 1);
}

} // namespace
} // namespace OpenGeoLab::Base
