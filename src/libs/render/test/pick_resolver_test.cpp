#include "pick_resolver.hpp"

#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/scene/pick_id.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Render::PickMode;
using OpenGeoLab::Render::PickResolver;
using OpenGeoLab::Render::PickResult;
using OpenGeoLab::Scene::PickId;
using OpenGeoLab::Scene::TopologyIndex;

TEST_CASE("PickResolver — empty input returns invalid") {
    TopologyIndex topo;
    PickResolver resolver(topo);
    auto result = resolver.resolve({}, PickMode::VEF);
    CHECK_FALSE(result.valid);
}

TEST_CASE("PickResolver — zero pickIds are skipped") {
    TopologyIndex topo;
    PickResolver resolver(topo);
    auto result = resolver.resolve({0, 0, 0}, PickMode::VEF);
    CHECK_FALSE(result.valid);
}

TEST_CASE("PickResolver — VEF mode: vertex wins over edge and face") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t face = PickId::encode(1, EntityType::GeoFace, 10);
    uint64_t edge = PickId::encode(1, EntityType::GeoEdge, 5);
    uint64_t vertex = PickId::encode(1, EntityType::GeoVertex, 2);

    auto result = resolver.resolve({face, edge, vertex}, PickMode::VEF);
    REQUIRE(result.valid);
    CHECK(result.entityType == EntityType::GeoVertex);
    CHECK(result.localId == 2);
    CHECK(result.shapeId == 1);
}

TEST_CASE("PickResolver — VEF mode: edge wins over face") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t face = PickId::encode(1, EntityType::GeoFace, 10);
    uint64_t edge = PickId::encode(1, EntityType::GeoEdge, 5);

    auto result = resolver.resolve({face, edge}, PickMode::VEF);
    REQUIRE(result.valid);
    CHECK(result.entityType == EntityType::GeoEdge);
    CHECK(result.localId == 5);
}

TEST_CASE("PickResolver — Part mode: returns shapeId") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t edge = PickId::encode(42, EntityType::GeoEdge, 7);
    auto result = resolver.resolve({edge}, PickMode::Part);
    REQUIRE(result.valid);
    CHECK(result.shapeId == 42);
}

TEST_CASE("PickResolver — resolveAll returns unique results") {
    TopologyIndex topo;
    PickResolver resolver(topo);

    uint64_t e1 = PickId::encode(1, EntityType::GeoEdge, 1);
    uint64_t e2 = PickId::encode(1, EntityType::GeoEdge, 2);
    uint64_t e1_dup = PickId::encode(1, EntityType::GeoEdge, 1);

    auto results = resolver.resolveAll({e1, e2, e1_dup}, PickMode::VEF);
    CHECK(results.size() == 2); // e1 de-duplicated
}
