/**
 * @file command_dispatcher_test.cpp
 * @brief Unit tests for CommandDispatcher
 */

#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/command/module_registry.hpp>
#include <opengeolab/core/module_data_event.hpp>

#include <doctest/doctest.h>

using Kangaroo::Util::PluginComponentFactory;
using OpenGeoLab::Command::CommandDispatcher;
using OpenGeoLab::Command::registerBuiltinModules;
using OpenGeoLab::Core::NO_PROGRESS_CALLBACK;

namespace {

void checkSchemaField(const nlohmann::json& schema, const bool require_required) {
    REQUIRE(schema.is_object());
    REQUIRE(schema.contains("type"));
    CHECK(schema["type"].is_string());
    REQUIRE(schema.contains("description"));
    CHECK(schema["description"].is_string());
    if(require_required) {
        REQUIRE(schema.contains("required"));
        CHECK(schema["required"].is_boolean());
    }
}

void checkActionDescribeSchema(const nlohmann::json& action) {
    REQUIRE(action.is_object());
    CHECK(action.contains("name"));
    CHECK(action.contains("description"));
    REQUIRE(action.contains("params"));
    REQUIRE(action.contains("returns"));

    const auto& params = action["params"];
    const auto& returns = action["returns"];
    REQUIRE(params.is_object());
    REQUIRE(returns.is_object());

    for(const auto& [param_name, param_schema] : params.items()) {
        INFO("param_name=" << param_name);
        checkSchemaField(param_schema, true);
    }

    REQUIRE(returns.contains("ok"));
    REQUIRE(returns.contains("action"));
    CHECK(returns["ok"]["type"] == "boolean");
    CHECK(returns["ok"]["description"] == "true when the action completes successfully.");
    CHECK(returns["action"]["type"] == "string");
    CHECK(returns["action"]["description"] == "Echo of the action name.");

    for(const auto& [field_name, field_schema] : returns.items()) {
        INFO("return_field=" << field_name);
        checkSchemaField(field_schema, false);
    }
}

} // namespace

TEST_CASE("CommandDispatcher dispatches to IOModule via request JSON (stub not-implemented)") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    const CommandDispatcher dispatcher(factory);
    CHECK(dispatcher.hasModule("io"));

    const nlohmann::json request = {
        {"module", "io"}, {"action", "read_brep"}, {"param", {{"path", "test.brep"}}}};
    auto result = dispatcher.dispatch(request, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    CHECK(result.contains("summary"));
}

TEST_CASE("CommandDispatcher returns error on missing module field") {
    PluginComponentFactory factory;
    const CommandDispatcher dispatcher(factory);

    const nlohmann::json request = {{"action", "read_brep"}};
    auto result = dispatcher.dispatch(request, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    CHECK(result.contains("summary"));
}

TEST_CASE("CommandDispatcher returns error for unknown module") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    const CommandDispatcher dispatcher(factory);
    const nlohmann::json request = {{"module", "nonexistent"}, {"action", "foo"}, {"param", {}}};
    auto result = dispatcher.dispatch(request, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    CHECK(result.contains("summary"));
}

TEST_CASE("CommandDispatcher hasModule returns false for unknown") {
    PluginComponentFactory factory;
    const CommandDispatcher dispatcher(factory);
    CHECK_FALSE(dispatcher.hasModule("nonexistent"));
}

TEST_CASE("CommandDispatcher listModules returns registered modules") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    const CommandDispatcher dispatcher(factory);
    auto modules = dispatcher.listModules();
    REQUIRE(modules.size() == 2);

    bool found_io = false;
    bool found_geometry = false;
    for(const auto& m : modules) {
        if(m.m_moduleName == "io") {
            found_io = true;
        }
        if(m.m_moduleName == "geometry") {
            found_geometry = true;
        }
    }
    CHECK(found_io);
    CHECK(found_geometry);
}

TEST_CASE("CommandDispatcher describe returns full system description") { // NOLINT
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    const CommandDispatcher dispatcher(factory);
    auto desc = dispatcher.describe();

    // request_schema
    REQUIRE(desc.contains("request_schema"));
    auto& schema = desc["request_schema"];
    CHECK(schema["type"] == "object");
    CHECK(schema["properties"].contains("module"));
    CHECK(schema["properties"].contains("action"));
    CHECK(schema["properties"].contains("param"));

    // modules
    REQUIRE(desc.contains("modules"));
    auto& modules = desc["modules"];
    REQUIRE(modules.is_array());
    REQUIRE(modules.size() == 2);

    // Find io module in the array
    const nlohmann::json* io_mod_ptr = nullptr;
    for(const auto& mod : modules) {
        if(mod["name"] == "io") {
            io_mod_ptr = &mod;
            break;
        }
    }
    REQUIRE(io_mod_ptr != nullptr);
    const auto& io_mod = *io_mod_ptr;
    CHECK(io_mod["name"] == "io");
    CHECK(io_mod.contains("description"));
    REQUIRE(io_mod.contains("actions"));
    REQUIRE(io_mod["actions"].is_array());
    REQUIRE(io_mod["actions"].size() >= 1);

    // First action should be read_brep
    bool found_read_brep = false;
    for(const auto& action : io_mod["actions"]) {
        if(action["name"] == "read_brep") {
            found_read_brep = true;
            CHECK(action.contains("description"));
            CHECK(action.contains("params"));
            CHECK(action.contains("returns"));
        }
    }
    CHECK(found_read_brep);
}

TEST_CASE("CommandDispatcher describe exposes normalized action schemas") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    const CommandDispatcher dispatcher(factory);
    const auto desc = dispatcher.describe();

    REQUIRE(desc.contains("modules"));
    REQUIRE(desc["modules"].is_array());

    for(const auto& module : desc["modules"]) {
        INFO("module=" << module["name"]);
        REQUIRE(module.contains("actions"));
        REQUIRE(module["actions"].is_array());

        for(const auto& action : module["actions"]) {
            INFO("action=" << action["name"]);
            checkActionDescribeSchema(action);
        }
    }
}

TEST_CASE("registerBuiltinModules is idempotent on same factory") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);
    // Second call must not throw
    CHECK_NOTHROW(registerBuiltinModules(factory));
    // Module count unchanged
    const CommandDispatcher dispatcher(factory);
    CHECK(dispatcher.listModules().size() == 2);
}

TEST_CASE("CommandDispatcher findModule returns shared_ptr for registered module") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    const CommandDispatcher dispatcher(factory);
    auto io_module = dispatcher.findModule("io");
    CHECK(io_module != nullptr);

    auto geo_module = dispatcher.findModule("geometry");
    CHECK(geo_module != nullptr);
}

TEST_CASE("CommandDispatcher findModule returns nullptr for unknown module") {
    PluginComponentFactory factory;
    const CommandDispatcher dispatcher(factory);
    auto result = dispatcher.findModule("nonexistent");
    CHECK(result == nullptr);
}

TEST_CASE("onModuleDataChanged fires callback when module emits dataChanged") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);
    CommandDispatcher dispatcher(factory);

    int call_count = 0;
    bool received_item_added = false;
    auto conn =
        dispatcher.onModuleDataChanged("geometry", [&](OpenGeoLab::Core::ModuleDataEvent event) {
            ++call_count;
            if(event == OpenGeoLab::Core::ModuleDataEvent::ItemAdded) {
                received_item_added = true;
            }
        });

    CHECK(conn.isConnected());

    // Trigger via create_box (add → topology update → tessellation = multiple events)
    const nlohmann::json req = {{"module", "geometry"},
                                {"action", "create_box"},
                                {"param", {{"width", 1.0}, {"height", 1.0}, {"depth", 1.0}}}};
    const auto result = dispatcher.dispatch(req, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    CHECK(call_count >= 1);
    CHECK(received_item_added);
}

TEST_CASE("onModuleDataChanged returns disconnected handle for unknown module") {
    PluginComponentFactory factory;
    CommandDispatcher dispatcher(factory);

    auto conn =
        dispatcher.onModuleDataChanged("nonexistent", [](OpenGeoLab::Core::ModuleDataEvent) {});
    CHECK_FALSE(conn.isConnected());
}
