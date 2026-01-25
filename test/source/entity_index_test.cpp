#include "geometry/entity_index.hpp"

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS_Shape.hxx>
#include <catch2/catch_test_macros.hpp>
#include <gp_Pnt.hxx>

namespace {
using OpenGeoLab::Geometry::EntityIndex;
using OpenGeoLab::Geometry::EntityType;
using OpenGeoLab::Geometry::GeometryEntity;

class TestEntity : public GeometryEntity {
public:
    explicit TestEntity(EntityType type, TopoDS_Shape shape = {})
        : GeometryEntity(type), m_type(type), m_shape(std::move(shape)) {}

    [[nodiscard]] EntityType entityType() const override { return m_type; }
    [[nodiscard]] const char* typeName() const override { return "TestEntity"; }
    [[nodiscard]] bool canAddChildType(EntityType /*child_type*/) const override { return false; }

    [[nodiscard]] bool canAddParentType(EntityType /*parent_type*/) const override { return false; }
    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_shape; }

private:
    EntityType m_type{EntityType::None};
    TopoDS_Shape m_shape;
};

[[nodiscard]] TopoDS_Shape makeVertexShape() {
    return BRepBuilderAPI_MakeVertex(gp_Pnt(0, 0, 0)).Shape();
}

TEST_CASE("EntityIndex - add and find") {
    EntityIndex index;

    TopoDS_Shape vertex_shape = makeVertexShape();
    auto entity = std::make_shared<TestEntity>(EntityType::Vertex, vertex_shape);

    REQUIRE(index.addEntity(entity));
    REQUIRE(index.entityCount() == 1);
    REQUIRE(index.entityCountByType(EntityType::Vertex) == 1);

    REQUIRE(index.findById(entity->entityId()) == entity);
    REQUIRE(index.findByUIDAndType(entity->entityUID(), entity->entityType()) == entity);
    REQUIRE(index.findByShape(vertex_shape) == entity);
}

TEST_CASE("EntityIndex - remove") {
    EntityIndex index;

    TopoDS_Shape vertex_shape = makeVertexShape();
    auto entity = std::make_shared<TestEntity>(EntityType::Vertex, vertex_shape);
    REQUIRE(index.addEntity(entity));

    REQUIRE(index.removeEntity(entity->entityId()));
    REQUIRE(index.entityCount() == 0);
    REQUIRE(index.entityCountByType(EntityType::Vertex) == 0);

    REQUIRE(index.findById(entity->entityId()) == nullptr);
    REQUIRE(index.findByUIDAndType(entity->entityUID(), entity->entityType()) == nullptr);
    REQUIRE(index.findByShape(vertex_shape) == nullptr);
    REQUIRE_FALSE(index.removeEntity(entity->entityId()));
}

TEST_CASE("EntityIndex - null shape is not indexed") {
    EntityIndex index;

    auto entity = std::make_shared<TestEntity>(EntityType::Edge, TopoDS_Shape{});
    REQUIRE(index.addEntity(entity));

    REQUIRE(index.findByShape(TopoDS_Shape{}) == nullptr);
    REQUIRE(index.findById(entity->entityId()) == entity);
}

} // namespace
