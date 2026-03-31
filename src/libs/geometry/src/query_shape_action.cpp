/**
 * @file query_shape_action.cpp
 * @brief QueryShapeAction — returns topology and bounding box of a shape
 */

#include <opengeolab/geometry/query_shape_action.hpp>
#include <opengeolab/geometry/shape_store.hpp>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

namespace OpenGeoLab::Geometry {

QueryShapeAction::QueryShapeAction(ShapeStore& store) : m_store(store) {}
QueryShapeAction::~QueryShapeAction() = default;

nlohmann::json QueryShapeAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Query topology info and bounding box of a shape."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier to inspect."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId", {{"type", "integer"}, {"description", "Queried shape identifier."}}},
          {"name", {{"type", "string"}, {"description", "Registered shape name."}}},
          {"topology", {{"type", "object"}, {"description", "Topology counts for the shape."}}},
          {"boundingBox",
           {{"type", "object"},
            {"description", "Axis-aligned bounding box as {min: [x,y,z], max: [x,y,z]}."}}}}}};
}

nlohmann::json QueryShapeAction::execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) {
    const auto shape_id = param.value("shapeId", static_cast<uint32_t>(0));
    const auto* entry = m_store.find(shape_id);
    if(!entry) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"summary", "Unknown shapeId"}};
    }

    if(progress) {
        progress(0.5, "Computing bounding box...");
    }

    Bnd_Box bbox;
    BRepBndLib::Add(entry->shape, bbox);
    double xmin{};
    double ymin{};
    double zmin{};
    double xmax{};
    double ymax{};
    double zmax{};
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", "query_shape"},
            {"shapeId", shape_id},
            {"name", entry->name},
            {"topology",
             {{"solids", entry->solidMap.Extent()},
              {"faces", entry->faceMap.Extent()},
              {"edges", entry->edgeMap.Extent()},
              {"vertices", entry->vertexMap.Extent()},
              {"wires", entry->wireMap.Extent()}}},
            {"boundingBox", {{"min", {xmin, ymin, zmin}}, {"max", {xmax, ymax, zmax}}}}};
}

} // namespace OpenGeoLab::Geometry
