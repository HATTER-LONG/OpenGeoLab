/**
 * @file list_shapes_action.cpp
 * @brief ListShapesAction — enumerates all shapes in ShapeStore
 */

#include <opengeolab/geometry/list_shapes_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopAbs_ShapeEnum.hxx>

namespace OpenGeoLab::Geometry {

/// Map OCC ShapeType enum to human-readable string.
static const char* shapeTypeToString(TopAbs_ShapeEnum type) {
    switch(type) {
    case TopAbs_COMPOUND:
        return "Compound";
    case TopAbs_COMPSOLID:
        return "CompSolid";
    case TopAbs_SOLID:
        return "Solid";
    case TopAbs_SHELL:
        return "Shell";
    case TopAbs_FACE:
        return "Face";
    case TopAbs_WIRE:
        return "Wire";
    case TopAbs_EDGE:
        return "Edge";
    case TopAbs_VERTEX:
        return "Vertex";
    default:
        return "Shape";
    }
}

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
            // Compute bounding box
            Bnd_Box bbox;
            BRepBndLib::Add(entry->shape, bbox);
            double xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0;
            if(!bbox.IsVoid()) {
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            }

            shapes.push_back(
                {{"shapeId", id},
                 {"name", entry->name},
                 {"shapeType", shapeTypeToString(entry->shape.ShapeType())},
                 {"hasTessellation", entry->visualData != nullptr},
                 {"topology",
                  {{"solids", entry->solidMap.Extent()},
                   {"faces", entry->faceMap.Extent()},
                   {"edges", entry->edgeMap.Extent()},
                   {"vertices", entry->vertexMap.Extent()},
                   {"wires", entry->wireMap.Extent()}}},
                 {"boundingBox", {{"min", {xmin, ymin, zmin}}, {"max", {xmax, ymax, zmax}}}}});
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "list_shapes"}, {"count", ids.size()}, {"shapes", shapes}};
}

} // namespace OpenGeoLab::Geometry
