#include <opengeolab/render/batch_utils.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Render::BatchUtils::ArrayBatch;
using OpenGeoLab::Render::BatchUtils::buildArrayBatch;
using OpenGeoLab::Render::BatchUtils::buildIndexedBatch;
using OpenGeoLab::Render::BatchUtils::IndexedBatch;
using OpenGeoLab::Scene::DrawRange;
using OpenGeoLab::Scene::PrimitiveTopology;

TEST_CASE("buildIndexedBatch — empty input") {
    std::vector<DrawRange> ranges;
    auto batch = buildIndexedBatch(ranges, [](const DrawRange&) { return true; });
    CHECK(batch.drawCount() == 0);
    CHECK(batch.counts.empty());
    CHECK(batch.offsets.empty());
}

TEST_CASE("buildIndexedBatch — all accepted") {
    std::vector<DrawRange> ranges(2);
    ranges[0].indexOffset = 0;
    ranges[0].indexCount = 36;
    ranges[0].topology = PrimitiveTopology::Triangles;
    ranges[1].indexOffset = 36;
    ranges[1].indexCount = 24;
    ranges[1].topology = PrimitiveTopology::Triangles;

    auto batch = buildIndexedBatch(ranges, [](const DrawRange&) { return true; });
    CHECK(batch.drawCount() == 2);
    CHECK(batch.counts[0] == 36);
    CHECK(batch.counts[1] == 24);
    CHECK(batch.offsets[0] == reinterpret_cast<const void*>(0 * sizeof(uint32_t)));
    CHECK(batch.offsets[1] == reinterpret_cast<const void*>(36 * sizeof(uint32_t)));
}

TEST_CASE("buildIndexedBatch — predicate filters some") {
    std::vector<DrawRange> ranges(3);
    ranges[0].indexOffset = 0;
    ranges[0].indexCount = 10;
    ranges[0].shapeId = 1;
    ranges[1].indexOffset = 10;
    ranges[1].indexCount = 20;
    ranges[1].shapeId = 2;
    ranges[2].indexOffset = 30;
    ranges[2].indexCount = 15;
    ranges[2].shapeId = 1;

    auto batch = buildIndexedBatch(ranges, [](const DrawRange& r) { return r.shapeId == 1; });
    CHECK(batch.drawCount() == 2);
    CHECK(batch.counts[0] == 10);
    CHECK(batch.counts[1] == 15);
}

TEST_CASE("buildArrayBatch — all accepted") {
    std::vector<DrawRange> ranges(2);
    ranges[0].vertexOffset = 0;
    ranges[0].vertexCount = 100;
    ranges[1].vertexOffset = 100;
    ranges[1].vertexCount = 50;

    auto batch = buildArrayBatch(ranges, [](const DrawRange&) { return true; });
    CHECK(batch.drawCount() == 2);
    CHECK(batch.firsts[0] == 0);
    CHECK(batch.counts[0] == 100);
    CHECK(batch.firsts[1] == 100);
    CHECK(batch.counts[1] == 50);
}

TEST_CASE("buildArrayBatch — empty after filter") {
    std::vector<DrawRange> ranges(1);
    ranges[0].vertexOffset = 0;
    ranges[0].vertexCount = 10;

    auto batch = buildArrayBatch(ranges, [](const DrawRange&) { return false; });
    CHECK(batch.drawCount() == 0);
}
