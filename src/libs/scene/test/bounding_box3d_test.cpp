/**
 * @file bounding_box3d_test.cpp
 * @brief Unit tests for BoundingBox3D
 */

#include <opengeolab/scene/bounding_box3d.hpp>

#include <doctest/doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace OpenGeoLab::Scene::Tests {

namespace {

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

} // namespace

TEST_CASE("BoundingBox3D default construction is invalid") {
    const BoundingBox3D box;

    CHECK_FALSE(box.isValid());
}

TEST_CASE("BoundingBox3D expand point initializes min and max") {
    BoundingBox3D box;

    box.expand(glm::vec3{1.0F, 2.0F, 3.0F});

    CHECK(box.isValid());
    checkVec3(box.min, glm::vec3{1.0F, 2.0F, 3.0F});
    checkVec3(box.max, glm::vec3{1.0F, 2.0F, 3.0F});
}

TEST_CASE("BoundingBox3D expand multiple points encloses all points") {
    BoundingBox3D box;

    box.expand(glm::vec3{1.0F, 2.0F, 3.0F});
    box.expand(glm::vec3{-4.0F, 5.0F, 0.0F});
    box.expand(glm::vec3{2.5F, -1.0F, 8.0F});

    checkVec3(box.min, glm::vec3{-4.0F, -1.0F, 0.0F});
    checkVec3(box.max, glm::vec3{2.5F, 5.0F, 8.0F});
}

TEST_CASE("BoundingBox3D expand other merges boxes") {
    BoundingBox3D box;
    box.expand(glm::vec3{0.0F, 1.0F, 2.0F});
    box.expand(glm::vec3{3.0F, 4.0F, 5.0F});

    BoundingBox3D other;
    other.expand(glm::vec3{-2.0F, 6.0F, 1.0F});
    other.expand(glm::vec3{1.0F, 7.0F, 9.0F});

    box.expand(other);

    checkVec3(box.min, glm::vec3{-2.0F, 1.0F, 1.0F});
    checkVec3(box.max, glm::vec3{3.0F, 7.0F, 9.0F});
}

TEST_CASE("BoundingBox3D center returns midpoint") {
    BoundingBox3D box;
    box.expand(glm::vec3{0.0F, 0.0F, 0.0F});
    box.expand(glm::vec3{4.0F, 6.0F, 8.0F});

    checkVec3(box.center(), glm::vec3{2.0F, 3.0F, 4.0F});
}

TEST_CASE("BoundingBox3D size returns axis extents") {
    BoundingBox3D box;
    box.expand(glm::vec3{0.0F, 0.0F, 0.0F});
    box.expand(glm::vec3{4.0F, 6.0F, 8.0F});

    checkVec3(box.size(), glm::vec3{4.0F, 6.0F, 8.0F});
}

TEST_CASE("BoundingBox3D diagonal returns diagonal length") {
    BoundingBox3D box;
    box.expand(glm::vec3{0.0F, 0.0F, 0.0F});
    box.expand(glm::vec3{4.0F, 6.0F, 8.0F});

    CHECK(box.diagonal() == doctest::Approx(std::sqrt(116.0F)));
}

TEST_CASE("BoundingBox3D transformed identity preserves bounds") {
    BoundingBox3D box;
    box.expand(glm::vec3{-1.0F, 2.0F, 3.0F});
    box.expand(glm::vec3{4.0F, 5.0F, 6.0F});

    const BoundingBox3D transformed_box = box.transformed(glm::mat4{1.0F});

    CHECK(transformed_box.isValid());
    checkVec3(transformed_box.min, box.min);
    checkVec3(transformed_box.max, box.max);
}

TEST_CASE("BoundingBox3D transformed translation offsets bounds") {
    BoundingBox3D box;
    box.expand(glm::vec3{-1.0F, 2.0F, 3.0F});
    box.expand(glm::vec3{4.0F, 5.0F, 6.0F});

    const glm::mat4 translation = glm::translate(glm::mat4{1.0F}, glm::vec3{10.0F, -3.0F, 2.0F});
    const BoundingBox3D transformed_box = box.transformed(translation);

    checkVec3(transformed_box.min, glm::vec3{9.0F, -1.0F, 5.0F});
    checkVec3(transformed_box.max, glm::vec3{14.0F, 2.0F, 8.0F});
}

} // namespace OpenGeoLab::Scene::Tests
