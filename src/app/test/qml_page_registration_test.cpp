/**
 * @file qml_page_registration_test.cpp
 * @brief Verifies geometry creation pages are registered in app QML resources.
 */

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>

namespace OpenGeoLab::App::Tests {

namespace {

namespace fs = std::filesystem;

std::string readFile(const fs::path& path) {
    std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void requireSnippets(const std::string& content, std::initializer_list<std::string_view> snippets) {
    for(const auto snippet : snippets) {
        INFO("Missing snippet: " << snippet);
        CHECK(content.find(snippet) != std::string::npos);
    }
}

fs::path appSourceDir() { return fs::path{OPENGEOLAB_APP_SOURCE_DIR}; }

} // namespace

TEST_CASE("MainPages registers all geometry creation pages") {
    const auto mainPagesPath = appSourceDir() / "resource/qml/MainPages.qml";
    REQUIRE_MESSAGE(fs::exists(mainPagesPath), "Missing file: " << mainPagesPath.string());

    const auto content = readFile(mainPagesPath);
    requireSnippets(content,
                    {"\"addBox\":      { path: \"components/pages/CreateBoxPage.qml\" }",
                     "\"addCylinder\": { path: \"components/pages/CreateCylinderPage.qml\" }",
                     "\"addSphere\":   { path: \"components/pages/CreateSpherePage.qml\" }",
                     "\"addTorus\":    { path: \"components/pages/CreateTorusPage.qml\" }"});
}

TEST_CASE("App CMake packages all geometry page QML files") {
    const auto cmakePath = appSourceDir() / "CMakeLists.txt";
    REQUIRE_MESSAGE(fs::exists(cmakePath), "Missing file: " << cmakePath.string());

    const auto content = readFile(cmakePath);
    requireSnippets(content, {"resource/qml/components/pages/CreateBoxPage.qml",
                              "resource/qml/components/pages/CreateCylinderPage.qml",
                              "resource/qml/components/pages/CreateSpherePage.qml",
                              "resource/qml/components/pages/CreateTorusPage.qml"});
}

TEST_CASE("CreateCylinderPage defines the expected cylinder workflow") {
    const auto pagePath = appSourceDir() / "resource/qml/components/pages/CreateCylinderPage.qml";
    REQUIRE_MESSAGE(fs::exists(pagePath), "Missing file: " << pagePath.string());

    const auto content = readFile(pagePath);
    requireSnippets(content, {"pageTitle: qsTr(\"Create Cylinder\")", "pageIcon: \"cylinder\"",
                              "actionId: \"addCylinder\"", "property string cylinderName: \"\"",
                              "property real centerX: 0.0", "property real centerY: 0.0",
                              "property real centerZ: 0.0", "property real radius: 5.0",
                              "property real cylHeight: 10.0", "label: qsTr(\"Cylinder Name\")",
                              "label: qsTr(\"Center Point\")", "tooltipText: qsTr(\"Radius\")",
                              "tooltipText: qsTr(\"Height\")", "text: qsTr(\"Volume:\")",
                              "text: qsTr(\"Surface Area:\")", "action: \"create_cylinder\"",
                              "radius: root.radius, height: root.cylHeight"});
}

TEST_CASE("CreateSpherePage defines the expected sphere workflow") {
    const auto pagePath = appSourceDir() / "resource/qml/components/pages/CreateSpherePage.qml";
    REQUIRE_MESSAGE(fs::exists(pagePath), "Missing file: " << pagePath.string());

    const auto content = readFile(pagePath);
    requireSnippets(content, {"pageTitle: qsTr(\"Create Sphere\")", "pageIcon: \"sphere\"",
                              "actionId: \"addSphere\"", "property string sphereName: \"\"",
                              "property real centerX: 0.0", "property real centerY: 0.0",
                              "property real centerZ: 0.0", "property real radius: 5.0",
                              "label: qsTr(\"Sphere Name\")", "label: qsTr(\"Center Point\")",
                              "tooltipText: qsTr(\"Radius\")", "text: qsTr(\"Volume:\")",
                              "text: qsTr(\"Surface Area:\")", "text: qsTr(\"Diameter:\")",
                              "action: \"create_sphere\"", "radius: root.radius"});
}

TEST_CASE("CreateTorusPage defines the expected torus workflow") {
    const auto pagePath = appSourceDir() / "resource/qml/components/pages/CreateTorusPage.qml";
    REQUIRE_MESSAGE(fs::exists(pagePath), "Missing file: " << pagePath.string());

    const auto content = readFile(pagePath);
    requireSnippets(content, {"pageTitle: qsTr(\"Create Torus\")",
                              "pageIcon: \"torus\"",
                              "actionId: \"addTorus\"",
                              "property string torusName: \"\"",
                              "property real centerX: 0.0",
                              "property real centerY: 0.0",
                              "property real centerZ: 0.0",
                              "property real majorRadius: 10.0",
                              "property real minorRadius: 3.0",
                              "label: qsTr(\"Torus Name\")",
                              "label: qsTr(\"Center Point\")",
                              "tooltipText: qsTr(\"Major Radius\")",
                              "tooltipText: qsTr(\"Minor Radius\")",
                              "visible: root.minorRadius >= root.majorRadius",
                              "text: qsTr(\"Minor radius should be less than major radius\")",
                              "text: qsTr(\"Volume:\")",
                              "text: qsTr(\"Surface Area:\")",
                              "text: qsTr(\"Outer Diameter:\")",
                              "action: \"create_torus\"",
                              "majorRadius: root.majorRadius, minorRadius: root.minorRadius"});
}

TEST_CASE("MainPages registers mesh pages") {
    const auto mainPagesPath = appSourceDir() / "resource/qml/MainPages.qml";
    REQUIRE_MESSAGE(fs::exists(mainPagesPath), "Missing file: " << mainPagesPath.string());

    const auto content = readFile(mainPagesPath);
    requireSnippets(content,
                    {"\"meshSurface\": { path: \"components/pages/MeshSurfacePage.qml\" }",
                     "\"meshVolume\":  { path: \"components/pages/MeshVolumePage.qml\" }"});
}

TEST_CASE("App CMake packages mesh page QML files") {
    const auto cmakePath = appSourceDir() / "CMakeLists.txt";
    REQUIRE_MESSAGE(fs::exists(cmakePath), "Missing file: " << cmakePath.string());

    const auto content = readFile(cmakePath);
    requireSnippets(content, {"resource/qml/components/pages/MeshSurfacePage.qml",
                              "resource/qml/components/pages/MeshVolumePage.qml",
                              "resource/qml/components/ShapeSelector.qml"});
}

TEST_CASE("MeshSurfacePage defines the expected surface mesh workflow") {
    const auto pagePath = appSourceDir() / "resource/qml/components/pages/MeshSurfacePage.qml";
    REQUIRE_MESSAGE(fs::exists(pagePath), "Missing file: " << pagePath.string());

    const auto content = readFile(pagePath);
    requireSnippets(content, {"pageIcon: \"meshSurface\"",
                              "actionId: \"meshSurface\"",
                              "action: \"generate_surface_mesh\"",
                              "property real minSize: 0.1",
                              "property real maxSize: 10.0",
                              "property int algorithm: 6",
                              "ShapeSelector"});
}

TEST_CASE("MeshVolumePage defines the expected volume mesh workflow") {
    const auto pagePath = appSourceDir() / "resource/qml/components/pages/MeshVolumePage.qml";
    REQUIRE_MESSAGE(fs::exists(pagePath), "Missing file: " << pagePath.string());

    const auto content = readFile(pagePath);
    requireSnippets(content, {"pageIcon: \"meshVolume\"",
                              "actionId: \"meshVolume\"",
                              "action: \"generate_volume_mesh\"",
                              "property real minSize: 0.1",
                              "property real maxSize: 10.0",
                              "property int algorithm: 1",
                              "property int optimizeAlgorithm: 0",
                              "ShapeSelector"});
}

TEST_CASE("Chinese translations cover the new geometry pages") {
    const auto translationPath = appSourceDir() / "resource/translations/opengeolab_zh_CN.ts";
    REQUIRE_MESSAGE(fs::exists(translationPath), "Missing file: " << translationPath.string());

    const auto content = readFile(translationPath);
    requireSnippets(content, {"<name>CreateCylinderPage</name>", "<source>Create Cylinder</source>",
                              "<source>Cylinder Name</source>", "<source>Center Point</source>",
                              "<source>Radius</source>", "<source>Height</source>",
                              "<source>Surface Area:</source>", "<name>CreateSpherePage</name>",
                              "<source>Create Sphere</source>", "<source>Sphere Name</source>",
                              "<source>Diameter:</source>", "<name>CreateTorusPage</name>",
                              "<source>Create Torus</source>", "<source>Torus Name</source>",
                              "<source>Major Radius</source>", "<source>Minor Radius</source>",
                              "<source>Outer Diameter:</source>",
                              "<source>Minor radius should be less than major radius</source>"});
}

} // namespace OpenGeoLab::App::Tests
