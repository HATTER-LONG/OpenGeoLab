/**
 * @file geometry_module_test.cpp
 * @brief Unit tests for GeometryModule and CreateBoxAction
 */

#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/delete_shape_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/geometry/import_brep_action.hpp>
#include <opengeolab/geometry/import_step_action.hpp>
#include <opengeolab/geometry/query_shape_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/tessellate_action.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <doctest/doctest.h>

TEST_CASE("GeometryModule describe returns module info with actions") {
    Kangaroo::Util::PluginComponentFactory factory;
    const OpenGeoLab::Geometry::GeometryModule mod(factory);
    auto desc = mod.describe();
    CHECK(desc["name"] == "geometry");
    CHECK(desc.contains("description"));
    CHECK(desc["actions"].is_array());
    CHECK(desc["actions"].size() == 11);

    // Verify create_box is present (order depends on factory enumeration)
    bool found_create_box = false;
    for(const auto& action : desc["actions"]) {
        if(action["name"] == "create_box") {
            found_create_box = true;
            break;
        }
    }
    CHECK(found_create_box);
}

TEST_CASE("GeometryModule dispatches create_box action") {
    Kangaroo::Util::PluginComponentFactory factory;
    const OpenGeoLab::Geometry::GeometryModule mod(factory);
    const nlohmann::json request = {{"module", "geometry"},
                                    {"action", "create_box"},
                                    {"param", {{"width", 2.0}, {"height", 3.0}, {"depth", 4.0}}}};

    std::vector<double> progress_values;
    auto progress_cb = [&](double p, const std::string&) {
        progress_values.push_back(p);
        return true;
    };

    auto result = mod.process(request, progress_cb);
    CHECK(result["ok"] == true);
    CHECK(result["action"] == "create_box");
    CHECK(result.contains("shapeId"));
    CHECK(result.contains("topology"));
    CHECK(result["topology"]["faces"] == 6);
    CHECK(result["topology"]["edges"] == 12);
    CHECK(result["topology"]["vertices"] == 8);
    CHECK(result["topology"]["solids"] == 1);

    // Should have progress calls: 0.0, 0.3, 0.5, 1.0
    CHECK(progress_values.size() == 4);
    CHECK(progress_values.front() == doctest::Approx(0.0));
    CHECK(progress_values.back() == doctest::Approx(1.0));
}

TEST_CASE("GeometryModule throws on missing action field") {
    Kangaroo::Util::PluginComponentFactory factory;
    const OpenGeoLab::Geometry::GeometryModule mod(factory);
    const nlohmann::json request = {{"module", "geometry"}, {"param", {{"width", 1.0}}}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("GeometryModule throws on unknown action") {
    Kangaroo::Util::PluginComponentFactory factory;
    const OpenGeoLab::Geometry::GeometryModule mod(factory);
    const nlohmann::json request = {{"module", "geometry"}, {"action", "unknown_action"}};
    CHECK_THROWS_AS((void)mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK),
                    std::invalid_argument);
}

TEST_CASE("CreateBoxAction uses default dimensions when params missing") {
    OpenGeoLab::Geometry::ShapeStore store;
    OpenGeoLab::Geometry::CreateBoxAction action(store);
    const nlohmann::json param = nlohmann::json::object();
    auto result = action.execute(param, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    CHECK(result["topology"]["faces"] == 6);
    CHECK(result["topology"]["solids"] == 1);
    CHECK(store.size() == 1);
}

TEST_CASE("CreateBoxAction with tessellate=false skips tessellation") {
    OpenGeoLab::Geometry::ShapeStore store;
    OpenGeoLab::Geometry::CreateBoxAction action(store);
    const nlohmann::json param = {{"width", 1.0}, {"tessellate", false}};
    auto result = action.execute(param, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    auto shape_id = result["shapeId"].get<uint32_t>();
    const auto* entry = store.find(shape_id);
    REQUIRE(entry != nullptr);
    CHECK(entry->visualData == nullptr);
}

TEST_CASE("Geometry actions include action in direct error responses") {
    using OpenGeoLab::Core::NO_PROGRESS_CALLBACK;
    OpenGeoLab::Geometry::ShapeStore store;

    SUBCASE("delete_shape unknown shapeId") {
        OpenGeoLab::Geometry::DeleteShapeAction action(store);
        const auto result = action.execute({{"shapeId", 999U}}, NO_PROGRESS_CALLBACK);
        CHECK(result["ok"] == false);
        CHECK(result["action"] == "delete_shape");
    }

    SUBCASE("query_shape unknown shapeId") {
        OpenGeoLab::Geometry::QueryShapeAction action(store);
        const auto result = action.execute({{"shapeId", 999U}}, NO_PROGRESS_CALLBACK);
        CHECK(result["ok"] == false);
        CHECK(result["action"] == "query_shape");
    }

    SUBCASE("tessellate unknown shapeId") {
        OpenGeoLab::Geometry::TessellateAction action(store);
        const auto result = action.execute({{"shapeId", 999U}}, NO_PROGRESS_CALLBACK);
        CHECK(result["ok"] == false);
        CHECK(result["action"] == "tessellate");
    }

    SUBCASE("import_step missing path") {
        OpenGeoLab::Geometry::ImportStepAction action(store);
        const auto result = action.execute(nlohmann::json::object(), NO_PROGRESS_CALLBACK);
        CHECK(result["ok"] == false);
        CHECK(result["action"] == "import_step");
    }

    SUBCASE("import_step file not found") {
        OpenGeoLab::Geometry::ImportStepAction action(store);
        const auto result = action.execute({{"path", "does_not_exist.step"}}, NO_PROGRESS_CALLBACK);
        CHECK(result["ok"] == false);
        CHECK(result["action"] == "import_step");
    }

    SUBCASE("import_brep missing path") {
        OpenGeoLab::Geometry::ImportBrepAction action(store);
        const auto result = action.execute(nlohmann::json::object(), NO_PROGRESS_CALLBACK);
        CHECK(result["ok"] == false);
        CHECK(result["action"] == "import_brep");
    }

    SUBCASE("import_brep file not found") {
        OpenGeoLab::Geometry::ImportBrepAction action(store);
        const auto result = action.execute({{"path", "does_not_exist.brep"}}, NO_PROGRESS_CALLBACK);
        CHECK(result["ok"] == false);
        CHECK(result["action"] == "import_brep");
    }
}

TEST_CASE("GeometryModule shapeStore is accessible") {
    Kangaroo::Util::PluginComponentFactory factory;
    OpenGeoLab::Geometry::GeometryModule mod(factory);
    CHECK(mod.shapeStore().size() == 0);

    // Process a create_box to add a shape
    const nlohmann::json request = {
        {"module", "geometry"}, {"action", "create_box"}, {"param", {{"width", 1.0}}}};
    auto result = mod.process(request, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    CHECK(result["ok"] == true);
    CHECK(mod.shapeStore().size() == 1);
}

TEST_CASE("list_shapes returns enhanced fields: shapeType, boundingBox, wires") {
    Kangaroo::Util::PluginComponentFactory factory;
    const OpenGeoLab::Geometry::GeometryModule mod(factory);

    // Create a box first
    const nlohmann::json create_req = {
        {"module", "geometry"},
        {"action", "create_box"},
        {"param", {{"width", 10.0}, {"height", 20.0}, {"depth", 5.0}}}};
    auto create_result = mod.process(create_req, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    REQUIRE(create_result["ok"] == true);

    // List shapes
    const nlohmann::json list_req = {
        {"module", "geometry"}, {"action", "list_shapes"}, {"param", nlohmann::json::object()}};
    auto list_result = mod.process(list_req, OpenGeoLab::Core::NO_PROGRESS_CALLBACK);
    REQUIRE(list_result["ok"] == true);
    REQUIRE(list_result["shapes"].size() == 1);

    auto& shape = list_result["shapes"][0];

    // shapeType
    CHECK(shape.contains("shapeType"));
    CHECK(shape["shapeType"].is_string());
    CHECK(shape["shapeType"] == "Solid");

    // topology.wires
    CHECK(shape["topology"].contains("wires"));
    CHECK(shape["topology"]["wires"].is_number());
    CHECK(shape["topology"]["wires"] == 6);

    // boundingBox
    CHECK(shape.contains("boundingBox"));
    CHECK(shape["boundingBox"].contains("min"));
    CHECK(shape["boundingBox"].contains("max"));
    CHECK(shape["boundingBox"]["min"].is_array());
    CHECK(shape["boundingBox"]["min"].size() == 3);
    CHECK(shape["boundingBox"]["max"].is_array());
    CHECK(shape["boundingBox"]["max"].size() == 3);

    // Box at origin with w=10, h=20, d=5 → min ~[0,0,0], max ~[10,20,5]
    auto max_bb = shape["boundingBox"]["max"];
    CHECK(max_bb[0].get<double>() == doctest::Approx(10.0).epsilon(0.01));
    CHECK(max_bb[1].get<double>() == doctest::Approx(20.0).epsilon(0.01));
    CHECK(max_bb[2].get<double>() == doctest::Approx(5.0).epsilon(0.01));
}
