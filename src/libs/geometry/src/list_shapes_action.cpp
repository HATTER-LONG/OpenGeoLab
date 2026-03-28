/**
 * @file list_shapes_action.cpp
 * @brief ListShapesAction — enumerates all shapes in ShapeStore
 */

#include <opengeolab/geometry/list_shapes_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

namespace OpenGeoLab::Geometry {

ListShapesAction::ListShapesAction(ShapeStore& store) : m_store(store) {}
ListShapesAction::~ListShapesAction() = default;

nlohmann::json ListShapesAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "List all shapes in ShapeStore."},
            {"params", nlohmann::json::object()}};
}

nlohmann::json ListShapesAction::execute(const nlohmann::json& /*param*/,
                                         const Core::ProgressCallback& progress) {
    if(progress) {
        progress(0.0, "Listing shapes...");
    }

    auto ids = m_store.allShapeIds();
    nlohmann::json shapes = nlohmann::json::array();
    for(auto id : ids) {
        const auto* entry = m_store.find(id);
        if(entry) {
            shapes.push_back({{"shapeId", id},
                              {"name", entry->name},
                              {"hasTessellation", entry->visualData != nullptr},
                              {"topology",
                               {{"solids", entry->solidMap.Extent()},
                                {"faces", entry->faceMap.Extent()},
                                {"edges", entry->edgeMap.Extent()},
                                {"vertices", entry->vertexMap.Extent()}}}});
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "list_shapes"}, {"count", ids.size()}, {"shapes", shapes}};
}

} // namespace OpenGeoLab::Geometry
