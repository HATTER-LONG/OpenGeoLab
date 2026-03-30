/**
 * @file pick_id_test.cpp
 * @brief Unit tests for PickId and DisplayModeMask
 */

#include <opengeolab/scene/display_mode.hpp>
#include <opengeolab/scene/pick_id.hpp>

#include <doctest/doctest.h>

#include <array>
#include <cstdint>

static_assert(OpenGeoLab::Scene::PickId::decodeShapeId(
                  OpenGeoLab::Scene::PickId::encode(42, OpenGeoLab::Core::EntityType::GeoFace, 3))
              == 42);
static_assert(OpenGeoLab::Scene::PickId::decodeLocalId(
                  OpenGeoLab::Scene::PickId::encode(42, OpenGeoLab::Core::EntityType::GeoFace, 3))
              == 3);

namespace OpenGeoLab::Scene::Tests {

TEST_CASE("PickId encodes and decodes shape id") {
    const uint64_t encoded = PickId::encode(42, Core::EntityType::GeoFace, 3);

    CHECK(PickId::decodeShapeId(encoded) == 42);
}

TEST_CASE("PickId encodes and decodes entity type") {
    const uint64_t encoded = PickId::encode(42, Core::EntityType::GeoFace, 3);

    CHECK(PickId::decodeType(encoded) == Core::EntityType::GeoFace);
}

TEST_CASE("PickId encodes and decodes local id") {
    const uint64_t encoded = PickId::encode(42, Core::EntityType::GeoFace, 3);

    CHECK(PickId::decodeLocalId(encoded) == 3);
}

TEST_CASE("PickId zero is invalid") {
    CHECK_FALSE(PickId::isValid(0));
}

TEST_CASE("PickId non-zero value is valid") {
    CHECK(PickId::isValid(PickId::encode(1, Core::EntityType::GeoVertex, 1)));
}

TEST_CASE("PickId preserves maximum shape id") {
    const uint64_t encoded = PickId::encode(0x00FF'FFFF, Core::EntityType::GeoEdge, 0);

    CHECK(PickId::decodeShapeId(encoded) == 0x00FF'FFFF);
}

TEST_CASE("PickId preserves maximum local id") {
    const uint64_t encoded = PickId::encode(1, Core::EntityType::GeoFace, 0xFFFF'FFFF);

    CHECK(PickId::decodeLocalId(encoded) == 0xFFFF'FFFF);
}

TEST_CASE("PickId round-trips all entity types") {
    constexpr std::array entity_types{
        Core::EntityType::GeoVertex,
        Core::EntityType::GeoEdge,
        Core::EntityType::GeoWire,
        Core::EntityType::GeoFace,
        Core::EntityType::GeoSolid,
        Core::EntityType::MeshNode,
        Core::EntityType::MeshEdge,
        Core::EntityType::MeshElement,
        Core::EntityType::SceneNode,
    };

    for (const Core::EntityType entity_type : entity_types) {
        const uint64_t encoded = PickId::encode(42, entity_type, 3);

        CHECK(PickId::decodeType(encoded) == entity_type);
        CHECK(PickId::decodeShapeId(encoded) == 42);
        CHECK(PickId::decodeLocalId(encoded) == 3);
    }
}

TEST_CASE("DisplayModeMask bitwise or combines flags") {
    const auto combined = DisplayModeMask::Surface | DisplayModeMask::Wireframe;

    CHECK(static_cast<uint8_t>(combined) == 0x03);
}

TEST_CASE("DisplayModeMask bitwise and preserves shared flags") {
    const auto combined = DisplayModeMask::Surface | DisplayModeMask::Wireframe;

    CHECK((combined & DisplayModeMask::Surface) == DisplayModeMask::Surface);
}

TEST_CASE("DisplayModeMask bitwise not flips bits") {
    CHECK((~DisplayModeMask::None) != DisplayModeMask::None);
}

} // namespace OpenGeoLab::Scene::Tests
