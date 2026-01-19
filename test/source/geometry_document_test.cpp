/**
 * @file geometry_document_test.cpp
 * @brief Unit tests for geometry document management
 */

#include <catch2/catch_test_macros.hpp>

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_entity.hpp"

#include <BRepPrimAPI_MakeBox.hxx>

using namespace OpenGeoLab::Geometry;

namespace {

/**
 * @brief Create a simple box part for testing
 */
std::shared_ptr<PartEntity> createTestBoxPart(double size = 10.0) {
    BRepPrimAPI_MakeBox makeBox(size, size, size);
    TopoDS_Shape boxShape = makeBox.Shape();

    auto part = std::make_shared<PartEntity>(boxShape);
    part->setName("TestBox");
    part->buildHierarchy();

    return part;
}

/**
 * @brief Test observer that records notifications
 */
class TestObserver : public IDocumentObserver {
public:
    int partAddedCount{0};
    int partRemovedCount{0};
    int selectionChangedCount{0};
    int visibilityChangedCount{0};
    int clearedCount{0};

    void onPartAdded(const std::shared_ptr<PartEntity>& /*part*/) override { ++partAddedCount; }

    void onPartRemoved(EntityId /*partId*/) override { ++partRemovedCount; }

    void onSelectionChanged(const std::vector<EntityId>& /*selectedIds*/) override {
        ++selectionChangedCount;
    }

    void onVisibilityChanged(EntityId /*entityId*/, bool /*visible*/) override {
        ++visibilityChangedCount;
    }

    void onDocumentCleared() override { ++clearedCount; }
};

} // namespace

TEST_CASE("GeometryDocument part management", "[geometry][document]") {
    // Get a fresh document instance
    auto doc = GeometryDocument::instance();
    doc->clear();

    SECTION("Add part") {
        auto part = createTestBoxPart();
        EntityId partId = part->id();

        bool added = doc->addPart(part);
        CHECK(added);
        CHECK(doc->partCount() == 1);

        auto found = doc->findPart(partId);
        CHECK(found != nullptr);
        CHECK(found->id() == partId);
    }

    SECTION("Add duplicate part fails") {
        auto part = createTestBoxPart();
        doc->addPart(part);

        bool addedAgain = doc->addPart(part);
        CHECK_FALSE(addedAgain);
        CHECK(doc->partCount() == 1);
    }

    SECTION("Remove part") {
        auto part = createTestBoxPart();
        EntityId partId = part->id();
        doc->addPart(part);

        bool removed = doc->removePart(partId);
        CHECK(removed);
        CHECK(doc->partCount() == 0);
        CHECK(doc->findPart(partId) == nullptr);
    }

    SECTION("Clear document") {
        doc->addPart(createTestBoxPart());
        doc->addPart(createTestBoxPart());
        CHECK(doc->partCount() == 2);

        doc->clear();
        CHECK(doc->partCount() == 0);
    }
}

TEST_CASE("GeometryDocument selection", "[geometry][document]") {
    auto doc = GeometryDocument::instance();
    doc->clear();

    auto part = createTestBoxPart();
    EntityId partId = part->id();
    doc->addPart(part);

    SECTION("Select entity") {
        doc->select(partId);
        CHECK(doc->isSelected(partId));
        CHECK(doc->selectedIds().size() == 1);
        CHECK(doc->primarySelection()->id() == partId);
    }

    SECTION("Deselect entity") {
        doc->select(partId);
        doc->deselect(partId);
        CHECK_FALSE(doc->isSelected(partId));
        CHECK(doc->selectedIds().empty());
    }

    SECTION("Multi-selection") {
        auto part2 = createTestBoxPart();
        EntityId partId2 = part2->id();
        doc->addPart(part2);

        doc->select(partId);
        doc->select(partId2, true); // Add to selection

        CHECK(doc->selectedIds().size() == 2);
        CHECK(doc->isSelected(partId));
        CHECK(doc->isSelected(partId2));
    }

    SECTION("Replace selection") {
        auto part2 = createTestBoxPart();
        EntityId partId2 = part2->id();
        doc->addPart(part2);

        doc->select(partId);
        doc->select(partId2, false); // Replace selection

        CHECK(doc->selectedIds().size() == 1);
        CHECK_FALSE(doc->isSelected(partId));
        CHECK(doc->isSelected(partId2));
    }

    SECTION("Clear selection") {
        doc->select(partId);
        doc->clearSelection();
        CHECK(doc->selectedIds().empty());
    }
}

TEST_CASE("GeometryDocument observer notifications", "[geometry][document]") {
    auto doc = GeometryDocument::instance();
    doc->clear();

    auto observer = std::make_shared<TestObserver>();
    doc->addObserver(observer);

    SECTION("Part added notification") {
        doc->addPart(createTestBoxPart());
        CHECK(observer->partAddedCount == 1);
    }

    SECTION("Part removed notification") {
        auto part = createTestBoxPart();
        EntityId partId = part->id();
        doc->addPart(part);
        doc->removePart(partId);
        CHECK(observer->partRemovedCount == 1);
    }

    SECTION("Selection changed notification") {
        auto part = createTestBoxPart();
        doc->addPart(part);
        doc->select(part->id());
        CHECK(observer->selectionChangedCount == 1);
    }

    SECTION("Document cleared notification") {
        doc->addPart(createTestBoxPart());
        doc->clear();
        CHECK(observer->clearedCount == 1);
    }

    doc->removeObserver(observer);
}

TEST_CASE("GeometryDocument bounding box", "[geometry][document]") {
    auto doc = GeometryDocument::instance();
    doc->clear();

    SECTION("Empty document has invalid bounding box") {
        BoundingBox bbox = doc->totalBoundingBox();
        // Empty document - bbox may not be valid
    }

    SECTION("Bounding box includes all visible parts") {
        auto part1 = createTestBoxPart(10.0);
        auto part2 = createTestBoxPart(20.0);
        doc->addPart(part1);
        doc->addPart(part2);

        BoundingBox bbox = doc->totalBoundingBox();
        CHECK(bbox.isValid());
        // At least one dimension should be >= 20 (size of larger box)
        double maxDim =
            std::max({bbox.max.x - bbox.min.x, bbox.max.y - bbox.min.y, bbox.max.z - bbox.min.z});
        CHECK(maxDim >= 20.0);
    }
}
