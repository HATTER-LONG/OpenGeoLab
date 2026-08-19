/**
 * @file rhi_viewport_test.cpp
 * @brief Tests for RhiViewport integration points and app build wiring.
 */

#include <opengeolab/app/rhi_viewport.hpp>
#include <opengeolab/app/rhi_viewport_renderer.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <doctest/doctest.h>

#include <QGuiApplication>
#include <QMouseEvent>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace OpenGeoLab::App::Tests {

namespace Fs = std::filesystem;

namespace {

std::string readFile(const Fs::path& path) {
    const std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

Fs::path appSourceDir() { return Fs::path{OPENGEOLAB_APP_SOURCE_DIR}; }

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

QGuiApplication& ensureGuiApplication() {
    if(QCoreApplication::instance() != nullptr) {
        return *static_cast<QGuiApplication*>(QCoreApplication::instance());
    }

    static int argc = 1;
    static char app_name[] = "opengeolab_rhi_viewport_test";
    static char* argv[] = {app_name, nullptr};
    static std::unique_ptr<QGuiApplication> app = std::make_unique<QGuiApplication>(argc, argv);
    return *app;
}

class TestRhiViewport final : public RhiViewport {
public:
    using RhiViewport::RhiViewport;
    using RhiViewport::mousePressEvent;
    using RhiViewport::mouseReleaseEvent;
};

} // namespace

TEST_CASE("App CMake packages RhiViewport sources and QML registration header") {
    const auto cmake_path = appSourceDir() / "CMakeLists.txt";
    REQUIRE_MESSAGE(Fs::exists(cmake_path), "Missing file: " << cmake_path.string());

    const auto content = readFile(cmake_path);
    CHECK(content.find("GuiPrivate") != std::string::npos);
    CHECK(content.find("src/rhi_viewport.cpp") != std::string::npos);
    CHECK(content.find("src/rhi_viewport_renderer.cpp") != std::string::npos);
    CHECK(content.find("include/opengeolab/app/rhi_viewport.hpp") != std::string::npos);
    CHECK(content.find("OpenGeoLab::Render") != std::string::npos);
    CHECK(content.find("Qt6::GuiPrivate") != std::string::npos);
}

TEST_CASE("ViewportPanel wires the RhiViewport QML item for interactive picking") {
    const auto qml_path = appSourceDir() / "resource/qml/sections/ViewportPanel.qml";
    REQUIRE_MESSAGE(Fs::exists(qml_path), "Missing file: " << qml_path.string());

    const auto content = readFile(qml_path);
    CHECK(content.find("required property AppTheme theme") != std::string::npos);
    CHECK(content.find("import OpenGeoLab.App") != std::string::npos);
    CHECK(content.find("RhiViewport") != std::string::npos);
    CHECK(content.find("anchors.fill: parent") != std::string::npos);
    CHECK(content.find("pickingEnabled: true") != std::string::npos);
    CHECK(content.find("pickMode: 0") != std::string::npos);
    CHECK(content.find("onEntityPicked") != std::string::npos);
    CHECK(content.find("onEntityHovered") != std::string::npos);
    CHECK(content.find("onPickCleared") != std::string::npos);
}

TEST_CASE("RhiViewport queues click picking and forwards pick signals") {
    static_cast<void>(ensureGuiApplication());

    TestRhiViewport viewport;
    viewport.setWidth(320.0);
    viewport.setHeight(200.0);

    int picked_count = 0;
    int hovered_count = 0;
    int cleared_count = 0;
    Render::PickResult picked_result;
    Render::PickResult hovered_result;

    QObject::connect(&viewport, &RhiViewport::entityPicked,
                     [&](int shape_id, int entity_type, int local_id) {
                         ++picked_count;
                         picked_result.shapeId = static_cast<uint32_t>(shape_id);
                         picked_result.entityType = static_cast<Core::EntityType>(entity_type);
                         picked_result.localId = static_cast<uint32_t>(local_id);
                         picked_result.valid = true;
                     });
    QObject::connect(&viewport, &RhiViewport::entityHovered,
                     [&](int shape_id, int entity_type, int local_id) {
                         ++hovered_count;
                         hovered_result.shapeId = static_cast<uint32_t>(shape_id);
                         hovered_result.entityType = static_cast<Core::EntityType>(entity_type);
                         hovered_result.localId = static_cast<uint32_t>(local_id);
                         hovered_result.valid = true;
                     });
    QObject::connect(&viewport, &RhiViewport::pickCleared, [&]() { ++cleared_count; });

    const QPointF click_position{42.0, 26.0};
    QMouseEvent press_event(QEvent::MouseButtonPress, click_position, click_position,
                            click_position, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    viewport.mousePressEvent(&press_event);

    QMouseEvent release_event(QEvent::MouseButtonRelease, click_position, click_position,
                              click_position, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    viewport.mouseReleaseEvent(&release_event);

    const auto pending_pick = viewport.consumePendingPick();
    CHECK(pending_pick.active);
    CHECK(pending_pick.x == doctest::Approx(42.0F));
    CHECK(pending_pick.y == doctest::Approx(26.0F));
    CHECK_FALSE(viewport.consumePendingPick().active);

    viewport.notifyPickResult({7, Core::EntityType::GeoFace, 11, true});
    CHECK(picked_count == 1);
    CHECK(picked_result.valid);
    CHECK(picked_result.shapeId == 7);
    CHECK(picked_result.entityType == Core::EntityType::GeoFace);
    CHECK(picked_result.localId == 11);

    viewport.notifyHoverResult({9, Core::EntityType::GeoEdge, 5, true});
    CHECK(hovered_count == 1);
    CHECK(hovered_result.valid);
    CHECK(hovered_result.shapeId == 9);
    CHECK(hovered_result.entityType == Core::EntityType::GeoEdge);
    CHECK(hovered_result.localId == 5);

    viewport.notifyPickResult({});
    CHECK(cleared_count == 1);
}

TEST_CASE("RhiViewport fits the camera to the scene and creates a renderer") {
    static_cast<void>(ensureGuiApplication());

    Scene::SceneGraph scene;
    const Scene::NodeId node_id = scene.addNode("box");
    REQUIRE(node_id != 0);
    Scene::SceneNode* node = scene.findNode(node_id);
    REQUIRE(node != nullptr);

    Scene::BoundingBox3D bounds;
    bounds.expand(glm::vec3{-2.0F, -1.0F, 3.0F});
    bounds.expand(glm::vec3{6.0F, 5.0F, 9.0F});
    node->setLocalBounds(bounds);
    node->markDirty();

    TestRhiViewport viewport;
    viewport.setSceneGraph(&scene);
    viewport.fitToScene();

    const Scene::CameraState fit_camera = scene.viewportState().camera();
    checkVec3(fit_camera.target, glm::vec3{2.0F, 2.0F, 6.0F});
    const float fit_distance = bounds.diagonal() * 1.1F;
    checkVec3(fit_camera.position, glm::vec3{2.0F, 2.0F, 6.0F + fit_distance});

    viewport.setViewPreset(static_cast<int>(Scene::ViewPreset::Top));
    const Scene::CameraState top_camera = scene.viewportState().camera();
    checkVec3(top_camera.position, glm::vec3{2.0F, 2.0F + fit_distance, 6.0F});
    checkVec3(top_camera.up, glm::vec3{0.0F, 0.0F, -1.0F});

    auto* renderer = viewport.createRenderer();
    CHECK(renderer != nullptr);
    auto* viewport_renderer = dynamic_cast<RhiViewportRenderer*>(renderer);
    CHECK(viewport_renderer != nullptr);
    if(viewport_renderer != nullptr) {
        delete viewport_renderer;
    }
}

TEST_CASE("RhiViewport toggles tessellation overlay state and emits change signal") {
    static_cast<void>(ensureGuiApplication());

    TestRhiViewport viewport;
    int changed_count = 0;
    QObject::connect(&viewport, &RhiViewport::showTessellationChanged, [&]() { ++changed_count; });

    CHECK_FALSE(viewport.showTessellation());

    viewport.setShowTessellation(true);
    CHECK(viewport.showTessellation());
    CHECK(changed_count == 1);

    viewport.setShowTessellation(true);
    CHECK(changed_count == 1);

    viewport.toggleShowTessellation();
    CHECK_FALSE(viewport.showTessellation());
    CHECK(changed_count == 2);
}

TEST_CASE("Viewport toolbar exposes tessellation overlay toggle wiring and assets") {
    const auto toolbar_path = appSourceDir() / "resource/qml/components/ViewportToolbar.qml";
    REQUIRE_MESSAGE(Fs::exists(toolbar_path), "Missing file: " << toolbar_path.string());

    const auto panel_path = appSourceDir() / "resource/qml/sections/ViewportPanel.qml";
    REQUIRE_MESSAGE(Fs::exists(panel_path), "Missing file: " << panel_path.string());

    const auto icon_path = appSourceDir() / "resource/icons/viewMesh.svg";
    REQUIRE_MESSAGE(Fs::exists(icon_path), "Missing file: " << icon_path.string());

    const auto translation_path = appSourceDir() / "resource/translations/opengeolab_zh_CN.ts";
    REQUIRE_MESSAGE(Fs::exists(translation_path), "Missing file: " << translation_path.string());

    const auto toolbar_content = readFile(toolbar_path);
    CHECK(toolbar_content.find("signal showTessellationToggled") != std::string::npos);
    CHECK(toolbar_content.find("property bool showTessellationActive: false") != std::string::npos);
    CHECK(toolbar_content.find("iconKind: \"viewMesh\"") != std::string::npos);
    CHECK(toolbar_content.find("tooltip: qsTr(\"Toggle tessellation wireframe\")") !=
          std::string::npos);
    CHECK(toolbar_content.find("toggled: root.showTessellationActive") != std::string::npos);
    CHECK(toolbar_content.find("onClicked: root.showTessellationToggled()") != std::string::npos);

    const auto panel_content = readFile(panel_path);
    CHECK(panel_content.find("showTessellationActive: viewport.showTessellation") !=
          std::string::npos);
    CHECK(panel_content.find("onShowTessellationToggled: viewport.toggleShowTessellation()") !=
          std::string::npos);

    const auto icon_content = readFile(icon_path);
    CHECK(icon_content.find("<polygon points=\"3,19 12,4 21,19\"") != std::string::npos);
    CHECK(icon_content.find("<circle cx=\"12\" cy=\"4\" r=\"1.5\"") != std::string::npos);

    const auto translation_content = readFile(translation_path);
    CHECK(translation_content.find("<name>ViewportToolbar</name>") != std::string::npos);
    CHECK(translation_content.find("<source>Toggle tessellation wireframe</source>") !=
          std::string::npos);
    CHECK(translation_content.find("<translation>切换离散网格线框</translation>") !=
          std::string::npos);
}

} // namespace OpenGeoLab::App::Tests
