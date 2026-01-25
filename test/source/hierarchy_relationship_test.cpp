#include "geometry/geometry_entity.hpp"

#include <TopoDS_Shape.hxx>
#include <catch2/catch_test_macros.hpp>

namespace {
using OpenGeoLab::Geometry::EntityType;
using OpenGeoLab::Geometry::GeometryEntity;
using OpenGeoLab::Geometry::GeometryEntityPtr;

class TestEntity final : public GeometryEntity {
public:
    explicit TestEntity(EntityType type) : GeometryEntity(type) {}

    [[nodiscard]] EntityType entityType() const override { return m_type; }
    [[nodiscard]] const char* typeName() const override { return "TestEntity"; }
    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_shape; }

private:
    EntityType m_type{EntityType::None};
    TopoDS_Shape m_shape;
};

TEST_CASE("GeometryEntity - child reference count and multi-parents") { // NOLINT
    auto parent1 = std::make_shared<TestEntity>(EntityType::Compound);
    auto parent2 = std::make_shared<TestEntity>(EntityType::Compound);
    auto child = std::make_shared<TestEntity>(EntityType::Edge);

    REQUIRE(child->parentCount() == 0);

    parent1->addChild(child);
    parent1->addChild(child);

    REQUIRE(parent1->childCount() == 1);
    REQUIRE(parent1->childReferenceCount(child) == 2);
    REQUIRE(parent1->totalChildReferenceCount() == 2);

    parent2->addChild(child);

    REQUIRE(child->parentCount() == 2);

    REQUIRE(parent1->removeChild(child));
    REQUIRE(parent1->childReferenceCount(child) == 1);
    REQUIRE(child->parentCount() == 2);

    REQUIRE(parent1->removeChild(child));
    REQUIRE(parent1->childReferenceCount(child) == 0);
    REQUIRE(parent1->childCount() == 0);
    REQUIRE(child->parentCount() == 1);

    REQUIRE(parent2->removeChild(child));
    REQUIRE(child->parentCount() == 0);
}

} // namespace
