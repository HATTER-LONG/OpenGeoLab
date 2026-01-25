#include "geometry/geometry_document.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
using OpenGeoLab::Geometry::EntityId;
using OpenGeoLab::Geometry::EntityType;
using OpenGeoLab::Geometry::GeometryDocument;
using OpenGeoLab::Geometry::GeometryEntity;

class TestEntity final : public GeometryEntity {
public:
    explicit TestEntity(EntityType type) : GeometryEntity(type), m_type(type) {}

    [[nodiscard]] EntityType entityType() const override { return m_type; }
    [[nodiscard]] const char* typeName() const override { return "TestEntity"; }
    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_shape; }

private:
    EntityType m_type{EntityType::None};
    TopoDS_Shape m_shape;
};
} // namespace

namespace {

TEST_CASE("GeometryEntity relations - multi-parent") {
    auto doc = GeometryDocument::create();

    auto p1 = std::make_shared<TestEntity>(EntityType::Part);
    auto p2 = std::make_shared<TestEntity>(EntityType::Part);
    auto c = std::make_shared<TestEntity>(EntityType::Face);

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
    auto e = std::make_shared<TestEntity>(EntityType::Part);
    REQUIRE(doc->addEntity(e));

    const EntityId id = e->entityId();
    REQUIRE_FALSE(e->addChild(id));
    REQUIRE_FALSE(e->addParent(id));
}

TEST_CASE("GeometryEntity relations - prevent cycles") {
    auto doc = GeometryDocument::create();

    auto a = std::make_shared<TestEntity>(EntityType::Part);
    auto b = std::make_shared<TestEntity>(EntityType::Part);
    auto c = std::make_shared<TestEntity>(EntityType::Part);

    REQUIRE(doc->addEntity(a));
    REQUIRE(doc->addEntity(b));
    REQUIRE(doc->addEntity(c));

    REQUIRE(a->addChild(b->entityId()));
    REQUIRE(b->addChild(c->entityId()));

    // Adding c -> a would create a cycle.
    REQUIRE_FALSE(c->addChild(a->entityId()));
}

TEST_CASE("GeometryEntity relations - auto cleanup expired") {
    auto doc = GeometryDocument::create();

    auto parent = std::make_shared<TestEntity>(EntityType::Part);
    auto child = std::make_shared<TestEntity>(EntityType::Face);

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
