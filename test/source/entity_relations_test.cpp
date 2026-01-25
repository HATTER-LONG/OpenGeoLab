#include "geometry/geometry_document.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
using OpenGeoLab::Geometry::EntityId;
using OpenGeoLab::Geometry::EntityType;
using OpenGeoLab::Geometry::GeometryDocument;
using OpenGeoLab::Geometry::GeometryEntity;

class TestEntity : public GeometryEntity {
public:
    explicit TestEntity(EntityType type) : GeometryEntity(type), m_type(type) {}

    [[nodiscard]] EntityType entityType() const override { return m_type; }
    [[nodiscard]] const char* typeName() const override { return "TestEntity"; }
    [[nodiscard]] bool canAddChildType(EntityType child_type) const override {
        switch(m_type) {
        case EntityType::Edge:
            return child_type == EntityType::Vertex;
        case EntityType::Wire:
            return child_type == EntityType::Edge;
        case EntityType::Face:
            return child_type == EntityType::Wire;
        case EntityType::Shell:
            return child_type == EntityType::Face;
        case EntityType::Solid:
            return child_type == EntityType::Shell;
        default:
            return false;
        }
    }

    [[nodiscard]] bool canAddParentType(EntityType parent_type) const override {
        switch(m_type) {
        case EntityType::Vertex:
            return parent_type == EntityType::Edge;
        case EntityType::Edge:
            return parent_type == EntityType::Wire;
        case EntityType::Wire:
            return parent_type == EntityType::Face;
        case EntityType::Face:
            return parent_type == EntityType::Shell;
        case EntityType::Shell:
            return parent_type == EntityType::Solid;
        default:
            return false;
        }
    }
    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_shape; }

private:
    EntityType m_type{EntityType::None};
    TopoDS_Shape m_shape;
};
} // namespace

namespace {

TEST_CASE("GeometryEntity relations - multi-parent") {
    auto doc = GeometryDocument::create();

    auto p1 = std::make_shared<TestEntity>(EntityType::Edge);
    auto p2 = std::make_shared<TestEntity>(EntityType::Edge);
    auto c = std::make_shared<TestEntity>(EntityType::Vertex);

    REQUIRE(doc->addEntity(p1));
    REQUIRE(doc->addEntity(p2));
    REQUIRE(doc->addEntity(c));

    REQUIRE(p1->addChild(c->entityId()));
    REQUIRE(p2->addChild(c->entityId()));

    REQUIRE(c->parentCount() == 2);
    REQUIRE(p1->childCount() == 1);
    REQUIRE(p2->childCount() == 1);
}

TEST_CASE("GeometryEntity relations - prevent self-parent") {
    auto doc = GeometryDocument::create();
    auto e = std::make_shared<TestEntity>(EntityType::Edge);
    REQUIRE(doc->addEntity(e));

    const EntityId id = e->entityId();
    REQUIRE_FALSE(e->addChild(id));
    REQUIRE_FALSE(e->addParent(id));
}

TEST_CASE("GeometryEntity relations - prevent invalid type edges") {
    auto doc = GeometryDocument::create();

    auto edge = std::make_shared<TestEntity>(EntityType::Edge);
    auto vertex = std::make_shared<TestEntity>(EntityType::Vertex);

    REQUIRE(doc->addEntity(edge));
    REQUIRE(doc->addEntity(vertex));

    REQUIRE(edge->addChild(vertex->entityId()));

    // Reverse direction is invalid by type constraints.
    REQUIRE_FALSE(vertex->addChild(edge->entityId()));
}

TEST_CASE("GeometryEntity relations - auto cleanup expired") {
    auto doc = GeometryDocument::create();

    auto parent = std::make_shared<TestEntity>(EntityType::Edge);
    auto child = std::make_shared<TestEntity>(EntityType::Vertex);

    REQUIRE(doc->addEntity(parent));
    REQUIRE(doc->addEntity(child));

    REQUIRE(parent->addChild(child->entityId()));
    REQUIRE(parent->childCount() == 1);

    // Remove child from the index; relationship should self-clean.
    REQUIRE(doc->removeEntity(child->entityId()));

    REQUIRE(parent->childCount() == 0);
    REQUIRE(parent->children().empty());
}

} // namespace
