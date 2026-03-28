/**
 * @file command_dispatcher_test.cpp
 * @brief Unit tests for CommandDispatcher
 */

#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/command/module_registry.hpp>

#include <doctest/doctest.h>

using Kangaroo::Util::PluginComponentFactory;
using OpenGeoLab::Command::CommandDispatcher;
using OpenGeoLab::Command::registerBuiltinModules;
using OpenGeoLab::Core::NO_PROGRESS_CALLBACK;

TEST_CASE("CommandDispatcher dispatches to IOModule via request JSON (stub not-implemented)") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    CommandDispatcher dispatcher(factory);
    CHECK(dispatcher.hasModule("io"));

    nlohmann::json request = {
        {"module", "io"}, {"action", "read_brep"}, {"param", {{"path", "test.brep"}}}};
    auto result = dispatcher.dispatch(request, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    CHECK(result.contains("summary"));
}

TEST_CASE("CommandDispatcher returns error on missing module field") {
    PluginComponentFactory factory;
    CommandDispatcher dispatcher(factory);

    nlohmann::json request = {{"action", "read_brep"}};
    auto result = dispatcher.dispatch(request, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    CHECK(result.contains("summary"));
}

TEST_CASE("CommandDispatcher returns error for unknown module") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    CommandDispatcher dispatcher(factory);
    nlohmann::json request = {
        {"module", "nonexistent"}, {"action", "foo"}, {"param", {}}};
    auto result = dispatcher.dispatch(request, NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == false);
    CHECK(result.contains("summary"));
}

TEST_CASE("CommandDispatcher hasModule returns false for unknown") {
    PluginComponentFactory factory;
    CommandDispatcher dispatcher(factory);
    CHECK_FALSE(dispatcher.hasModule("nonexistent"));
}

TEST_CASE("CommandDispatcher listModules returns registered modules") {
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    CommandDispatcher dispatcher(factory);
    auto modules = dispatcher.listModules();
    REQUIRE(modules.size() == 2);

    bool found_io = false;
    bool found_geometry = false;
    for(const auto& m : modules) {
        if(m.m_moduleName == "io")
            found_io = true;
        if(m.m_moduleName == "geometry")
            found_geometry = true;
    }
    CHECK(found_io);
    CHECK(found_geometry);
}

TEST_CASE("CommandDispatcher describe returns full system description") { // NOLINT
    PluginComponentFactory factory;
    registerBuiltinModules(factory);

    CommandDispatcher dispatcher(factory);
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
