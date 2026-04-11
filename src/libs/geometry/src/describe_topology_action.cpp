/**
 * @file describe_topology_action.cpp
 * @brief DescribeTopologyAction — shape topology overview for LLM context
 */

#include <opengeolab/geometry/describe_topology_action.hpp>

#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>

namespace OpenGeoLab::Geometry {

DescribeTopologyAction::DescribeTopologyAction(ShapeStore& store) : m_store(store) {}
DescribeTopologyAction::~DescribeTopologyAction() = default;

nlohmann::json DescribeTopologyAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Return a structured overview of a shape's topology: face/edge/vertex "
                        "counts and per-entity summary with type, coordinates, and dimensions."},
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
          {"shapeName", {{"type", "string"}, {"description", "Registered shape name."}}},
          {"boundingBox",
           {{"type", "object"}, {"description", "AABB as {min: [x,y,z], max: [x,y,z]}."}}},
          {"counts",
           {{"type", "object"}, {"description", "Topology counts: {faces, edges, vertices}."}}},
          {"faces",
           {{"type", "array"},
            {"description", "Per-face summary: localId, surfaceType, center, normal, area, "
                            "and optional axis/radius for curved faces."}}},
          {"edges",
           {{"type", "array"},
            {"description", "Per-edge summary: localId, curveType, start, end, length, "
                            "and optional center/radius for curved edges."}}}}}};
}

nlohmann::json DescribeTopologyAction::execute(const nlohmann::json& param,
                                               const Core::ProgressCallback& progress) {
    if(!param.contains("shapeId") || !param["shapeId"].is_number()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Missing or invalid 'shapeId' parameter."}};
    }

    const auto shape_id = param["shapeId"].get<uint32_t>();
    const auto* entry = m_store.find(shape_id);
    if(!entry) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Unknown shapeId."}};
    }

    if(progress) {
        progress(0.0, "Extracting topology...");
    }

    // Bounding box
    Bnd_Box bbox;
    BRepBndLib::Add(entry->shape, bbox);
    nlohmann::json bb_json;
    if(!bbox.IsVoid()) {
        Standard_Real x_min = 0;
        Standard_Real y_min = 0;
        Standard_Real z_min = 0;
        Standard_Real x_max = 0;
        Standard_Real y_max = 0;
        Standard_Real z_max = 0;
        bbox.Get(x_min, y_min, z_min, x_max, y_max, z_max);
        bb_json = {{"min", {x_min, y_min, z_min}}, {"max", {x_max, y_max, z_max}}};
    }

    // Faces
    nlohmann::json faces_json = nlohmann::json::array();
    for(int i = 1; i <= entry->faceMap.Extent(); ++i) {
        faces_json.push_back(
            toJson(extractFaceInfo(static_cast<uint32_t>(i), TopoDS::Face(entry->faceMap(i)))));
    }

    if(progress) {
        progress(0.5, "Extracting edges...");
    }

    // Edges
    nlohmann::json edges_json = nlohmann::json::array();
    for(int i = 1; i <= entry->edgeMap.Extent(); ++i) {
        edges_json.push_back(
            toJson(extractEdgeInfo(static_cast<uint32_t>(i), TopoDS::Edge(entry->edgeMap(i)))));
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true},
            {"action", ACTION_NAME},
            {"shapeId", shape_id},
            {"shapeName", entry->name},
            {"boundingBox", std::move(bb_json)},
            {"counts",
             {{"faces", entry->faceMap.Extent()},
              {"edges", entry->edgeMap.Extent()},
              {"vertices", entry->vertexMap.Extent()}}},
            {"faces", std::move(faces_json)},
            {"edges", std::move(edges_json)}};
}

} // namespace OpenGeoLab::Geometry
