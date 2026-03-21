#include <doctest/doctest.h>

#include <opengeolab/base/module_service_interface.hpp>
#include <opengeolab/command/module_dispatcher.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/geometry/point_store.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace OpenGeoLab::Geometry {
namespace {

/// Force ogl_geometry DLL load so static registrations execute.
/// The volatile variable prevents the optimizer from discarding the DLL reference.
static volatile bool kForceLoad = [] {
    PointStore store;
    return store.empty();
}();

TEST_CASE("Geometry module registers with factory on load") {
    auto& factory = Kangaroo::Util::PluginComponentFactory::instance();
    auto module = factory.getSharedInstance<Base::IModuleService>("geometry");

    REQUIRE(module != nullptr);
    CHECK(module->moduleName() == "geometry");

    const auto actions = module->supportedActions();
    CHECK(actions.size() == 3);
}

TEST_CASE("Geometry module dispatches bounding_box action") {
    auto& factory = Kangaroo::Util::PluginComponentFactory::instance();
    Command::ModuleDispatcher dispatcher(factory);

    const auto response = nlohmann::json::parse(dispatcher.dispatch(
        R"({"module":"geometry","action":"bounding_box","requestId":"t1","payload":{"pointCount":32,"seed":7}})"));

    REQUIRE(response.at("ok") == true);
    CHECK(response.at("module") == "geometry");
    CHECK(response.at("action") == "bounding_box");
    CHECK(response.at("requestId") == "t1");
    CHECK(response.at("summary") == "Computed bounding box.");
    REQUIRE(response.at("result").is_object());
    CHECK(response.at("result").at("pointCount") == 32);
    CHECK(response.at("result").at("elapsedMs").is_number());
    CHECK(response.at("result").at("min").is_object());
    CHECK(response.at("result").at("max").is_object());
}

TEST_CASE("Geometry module dispatches set_points and get_stored_bbox") {
    auto& factory = Kangaroo::Util::PluginComponentFactory::instance();
    Command::ModuleDispatcher dispatcher(factory);

    const auto set_response = nlohmann::json::parse(dispatcher.dispatch(
        R"({"module":"geometry","action":"set_points","requestId":"t2","payload":{
            "points":[{"x":1,"y":2,"z":3},{"x":4,"y":5,"z":6}]}})"));

    REQUIRE(set_response.at("ok") == true);
    CHECK(set_response.at("result").at("pointCount") == 2);
    CHECK(set_response.at("result").at("stored") == true);

    const auto get_response = nlohmann::json::parse(dispatcher.dispatch(
        R"({"module":"geometry","action":"get_stored_bbox","requestId":"t3","payload":{}})"));

    REQUIRE(get_response.at("ok") == true);
    CHECK(get_response.at("result").at("pointCount") == 2);
    CHECK(get_response.at("result").at("boundingBox").at("min").at("x") == 1.0);
    CHECK(get_response.at("result").at("boundingBox").at("max").at("x") == 4.0);
}

TEST_CASE("Geometry module returns error for unknown action") {
    auto& factory = Kangaroo::Util::PluginComponentFactory::instance();
    Command::ModuleDispatcher dispatcher(factory);

    const auto response = nlohmann::json::parse(dispatcher.dispatch(
        R"({"module":"geometry","action":"nonexistent","requestId":"t4","payload":{}})"));

    CHECK(response.at("ok") == false);
    CHECK(response.at("module") == "geometry");
    CHECK(response.at("action") == "nonexistent");
}

} // namespace
} // namespace OpenGeoLab::Geometry
