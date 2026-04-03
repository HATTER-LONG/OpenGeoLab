/**
 * @file entity_ref_test.cpp
 * @brief Unit tests for EntityRef
 */

#include <opengeolab/core/entity_ref.hpp>

#include <doctest/doctest.h>

#include <unordered_set>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;

TEST_SUITE("EntityRef") {
    TEST_CASE("default is invalid") {
        EntityRef ref;
        CHECK_FALSE(ref.isValid());
    }

    TEST_CASE("valid geometry ref") {
        EntityRef ref{1, EntityType::GeoFace, 3};
        CHECK(ref.isValid());
        CHECK(ref.isGeometry());
        CHECK_FALSE(ref.isMesh());
    }

    TEST_CASE("shapeId 0 is valid") {
        EntityRef ref{0, EntityType::GeoEdge, 1};
        CHECK(ref.isValid());
        CHECK(ref.isGeometry());
    }

    TEST_CASE("valid mesh ref") {
        EntityRef ref{2, EntityType::MeshNode, 5};
        CHECK(ref.isValid());
        CHECK_FALSE(ref.isGeometry());
        CHECK(ref.isMesh());
    }

    TEST_CASE("equality") {
        EntityRef a{1, EntityType::GeoEdge, 2};
        EntityRef b{1, EntityType::GeoEdge, 2};
        EntityRef c{1, EntityType::GeoEdge, 3};
        CHECK(a == b);
        CHECK(a != c);
    }

    TEST_CASE("ordering") {
        EntityRef a{1, EntityType::GeoVertex, 1};
        EntityRef b{1, EntityType::GeoEdge, 1};
        CHECK(a < b);
    }

    TEST_CASE("tag extraction") {
        EntityRef ref{5, EntityType::GeoFace, 7};
        auto tag = ref.tag();
        CHECK(tag.type == EntityType::GeoFace);
        CHECK(tag.localId == 7);
    }

    TEST_CASE("usable in unordered_set") {
        std::unordered_set<EntityRef> set;
        set.insert({1, EntityType::GeoFace, 3});
        set.insert({1, EntityType::GeoFace, 3});
        set.insert({2, EntityType::GeoEdge, 1});
        CHECK(set.size() == 2);
    }
}
