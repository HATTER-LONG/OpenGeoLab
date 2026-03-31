#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/scene/geometry_scene_bridge.hpp>
#include <BRepPrimAPI_MakeBox.hxx>
#include <iostream>

int main() {
    using namespace OpenGeoLab;
    Geometry::ShapeStore store;
    Scene::SceneGraph scene;
    Scene::TopologyIndex topoIndex;
    Scene::GeometrySceneBridge bridge(scene, store, topoIndex);

    // Add and tessellate
    auto shapeId = store.add("TestBox", BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape());
    store.tessellate(shapeId);

    auto* node = scene.root()->children().front().get();
    auto* pickComp = node->pickComponent();
    
    std::cout << "Before update - pickComp address: " << pickComp << std::endl;
    std::cout << "Before update - pickEntries size: " << pickComp->pickEntries().size() << std::endl;

    // Trigger update with different tessellation - might replace render component
    store.tessellate(shapeId, Geometry::TessellationParams{0.05, 0.25});

    // Try to use the old pickComp pointer (DANGER!)
    std::cout << "After update - old pickComp address: " << pickComp << std::endl;
    std::cout << "After update - pickEntries size from OLD pointer: " << pickComp->pickEntries().size() << std::endl;
    
    // Get new pickComp
    auto* newPickComp = node->pickComponent();
    std::cout << "After update - new pickComp address: " << newPickComp << std::endl;
    std::cout << "After update - pickEntries size from NEW pointer: " << newPickComp->pickEntries().size() << std::endl;

    return 0;
}
