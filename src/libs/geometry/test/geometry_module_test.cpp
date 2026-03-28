/**
 * @file geometry_module_test.cpp
 * @brief Unit tests for GeometryModule and CreateBoxAction
 */

#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <doctest/doctest.h>

TEST_CASE("GeometryModule describe returns module info with actions") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Geometry::GeometryModule mod(factory);
    auto desc = mod.describe();
    CHECK(desc["name"] == "geometry");
    CHECK(desc.contains("description"));
    CHECK(desc["actions"].is_array());
    CHECK(desc["actions"].size() == 10);

    // Verify create_box is present (order depends on factory enumeration)
    bool foundCreateBox = false;
    for(const auto& action : desc["actions"]) {
        if(action["name"] == "create_box") {
            foundCreateBox = true;
            break;
        }
    }
    CHECK(foundCreateBox);
}

TEST_CASE("GeometryModule dispatches create_box action") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Geometry::GeometryModule mod(factory);
    nlohmann::json request = {{"module", "geometry"},
                              {"action", "create_box"},
                              {"param", {{"width", 2.0}, {"height", 3.0}, {"depth", 4.0}}}};

    std::vector<double> progressValues;
    auto progressCb = [&](double p, const std::string&) {
        progressValues.push_back(p);
        return true;
    };

    auto result = mod.process(request, progressCb);
    CHECK(result["ok"] == true);
    CHECK(result["action"] == "create_box");
    CHECK(result.contains("shapeId"));
    CHECK(result.contains("topology"));
    CHECK(result["topology"]["faces"] == 6);
    CHECK(result["topology"]["edges"] == 12);
    CHECK(result["topology"]["vertices"] == 8);
    CHECK(result["topology"]["solids"] == 1);

    // Should have progress calls: 0.0, 0.3, 0.5, 1.0
    CHECK(progressValues.size() == 4);
    CHECK(progressValues.front() == doctest::Approx(0.0));
    CHECK(progressValues.back() == doctest::Approx(1.0));
}

TEST_CASE("GeometryModule throws on missing action field") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Geometry::GeometryModule mod(factory);
    nlohmann::json request = {{"module", "geometry"}, {"param", {{"width", 1.0}}}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("GeometryModule throws on unknown action") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Geometry::GeometryModule mod(factory);
    nlohmann::json request = {{"module", "geometry"}, {"action", "unknown_action"}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("CreateBoxAction uses default dimensions when params missing") {
    OpenGeoLab::Geometry::ShapeStore store;
    OpenGeoLab::Geometry::CreateBoxAction action(store);
    nlohmann::json param = nlohmann::json::object();
    auto result = action.execute(param, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    CHECK(result["topology"]["faces"] == 6);
    CHECK(result["topology"]["solids"] == 1);
    CHECK(store.size() == 1);
}

TEST_CASE("CreateBoxAction with tessellate=false skips tessellation") {
    OpenGeoLab::Geometry::ShapeStore store;
    OpenGeoLab::Geometry::CreateBoxAction action(store);
    nlohmann::json param = {{"width", 1.0}, {"tessellate", false}};
    auto result = action.execute(param, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    auto shapeId = result["shapeId"].get<uint32_t>();
    const auto* entry = store.find(shapeId);
    REQUIRE(entry != nullptr);
    CHECK(entry->visualData == nullptr);
}

TEST_CASE("GeometryModule shapeStore is accessible") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Geometry::GeometryModule mod(factory);
    CHECK(mod.shapeStore().size() == 0);

    // Process a create_box to add a shape
    nlohmann::json request = {
        {"module", "geometry"}, {"action", "create_box"}, {"param", {{"width", 1.0}}}};
    auto result = mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    CHECK(mod.shapeStore().size() == 1);
}
