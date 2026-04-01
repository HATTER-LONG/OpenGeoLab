/**
 * @file label_manager_test.cpp
 * @brief Unit tests for LabelManager
 */

#include <opengeolab/scene/label_manager.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::EntityRef;
using OpenGeoLab::Core::EntityType;
using OpenGeoLab::Scene::LabelManager;

namespace {
constexpr EntityRef FACE_1{1, EntityType::GeoFace, 3};
constexpr EntityRef EDGE_1{1, EntityType::GeoEdge, 2};
} // namespace

TEST_SUITE("LabelManager") {
    TEST_CASE("initially empty") {
        const LabelManager mgr;
        CHECK(mgr.labels().empty());
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("add label") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});

        auto labels = mgr.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].entity == FACE_1);
        CHECK(labels[0].text == "F:3");
        CHECK(mgr.version() == 1);
    }

    TEST_CASE("add label for same entity replaces") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});
        mgr.addLabel({FACE_1, "Face 3"});

        auto labels = mgr.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].text == "Face 3");
        CHECK(mgr.version() == 2);
    }

    TEST_CASE("remove by entity") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});
        mgr.addLabel({EDGE_1, "E:2"});
        mgr.removeByEntity(FACE_1);

        auto labels = mgr.labels();
        REQUIRE(labels.size() == 1);
        CHECK(labels[0].entity == EDGE_1);
        CHECK(mgr.version() == 3);
    }

    TEST_CASE("remove nonexistent is no-op") {
        LabelManager mgr;
        mgr.removeByEntity(FACE_1);
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("clear labels") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});
        mgr.addLabel({EDGE_1, "E:2"});
        mgr.clearLabels();

        CHECK(mgr.labels().empty());
        CHECK(mgr.version() == 3);
    }

    TEST_CASE("clear empty is no-op") {
        LabelManager mgr;
        mgr.clearLabels();
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("invalid entity rejected") {
        LabelManager mgr;
        mgr.addLabel({EntityRef{}, "bad"});
        CHECK(mgr.labels().empty());
        CHECK(mgr.version() == 0);
    }

    TEST_CASE("signal emitted on add") {
        LabelManager mgr;
        int signal_count = 0;
        auto conn = mgr.labelsChanged.connect([&]() { ++signal_count; });

        mgr.addLabel({FACE_1, "F:3"});
        CHECK(signal_count == 1);
    }

    TEST_CASE("signal emitted on remove") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});

        int signal_count = 0;
        auto conn = mgr.labelsChanged.connect([&]() { ++signal_count; });

        mgr.removeByEntity(FACE_1);
        CHECK(signal_count == 1);
    }

    TEST_CASE("signal not emitted on no-op remove") {
        LabelManager mgr;
        int signal_count = 0;
        auto conn = mgr.labelsChanged.connect([&]() { ++signal_count; });

        mgr.removeByEntity(FACE_1);
        CHECK(signal_count == 0);
    }

    TEST_CASE("signal emitted on clear") {
        LabelManager mgr;
        mgr.addLabel({FACE_1, "F:3"});

        int signal_count = 0;
        auto conn = mgr.labelsChanged.connect([&]() { ++signal_count; });

        mgr.clearLabels();
        CHECK(signal_count == 1);
    }

    TEST_CASE("signal not emitted on clear empty") {
        LabelManager mgr;
        int signal_count = 0;
        auto conn = mgr.labelsChanged.connect([&]() { ++signal_count; });

        mgr.clearLabels();
        CHECK(signal_count == 0);
    }
}

TEST_SUITE("SceneGraph::labelManager") {
    TEST_CASE("accessible from SceneGraph") {
        OpenGeoLab::Scene::SceneGraph graph;
        graph.labelManager().addLabel({FACE_1, "F:3"});
        CHECK(graph.labelManager().labels().size() == 1);
    }
}
