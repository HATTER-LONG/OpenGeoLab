/**
 * @file vertex_array_object_test.cpp
 * @brief Unit tests for VertexArrayObject API contracts.
 */
#include <opengeolab/render/vertex_array_object.hpp>

#include <doctest/doctest.h>

#include <type_traits>

using OpenGeoLab::Render::VertexArrayObject;

TEST_CASE("VertexArrayObject") {
    SUBCASE("default constructed object is invalid") {
        const VertexArrayObject vao;
        CHECK_FALSE(vao.isValid());
    }

    SUBCASE("wrapper is move only") {
        CHECK(std::is_move_constructible_v<VertexArrayObject>);
        CHECK(std::is_move_assignable_v<VertexArrayObject>);
        CHECK_FALSE(std::is_copy_constructible_v<VertexArrayObject>);
        CHECK_FALSE(std::is_copy_assignable_v<VertexArrayObject>);
    }
}
