/**
 * @file query_entity_info_action.cpp
 * @brief QueryEntityInfoAction — detailed info for a single face/edge/vertex
 */

#include <opengeolab/geometry/query_entity_info_action.hpp>

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <TopoDS.hxx>

#include <algorithm>
#include <set>

namespace OpenGeoLab::Geometry {

QueryEntityInfoAction::QueryEntityInfoAction(ShapeStore& store) : m_store(store) {}
QueryEntityInfoAction::~QueryEntityInfoAction() = default;

nlohmann::json QueryEntityInfoAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Return detailed information about a single face, edge, or vertex, "
                            "including type-specific properties, bounding box, and adjacency."},
            {"params",
             {{"shapeId",
               {{"type", "integer"}, {"required", true}, {"description", "Shape identifier."}}},
              {"entityType",
               {{"type", "string"},
                {"required", true},
                {"description", "Entity type: 'face', 'edge', or 'vertex'."}}},
              {"localId",
               {{"type", "integer"},
                {"required", true},
                {"description", "1-based local index within the shape."}}}}},
            {"returns",
             {{"ok", {{"type", "boolean"}, {"description", "true on success."}}},
              {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
              {"shapeId", {{"type", "integer"}}},
              {"entityType", {{"type", "string"}}},
              {"localId", {{"type", "integer"}}},
              {"boundingBox", {{"type", "object"}}},
              {"adjacentEdges",
               {{"type", "array"}, {"description", "For face/vertex: adjacent edge localIds."}}},
              {"adjacentFaces",
               {{"type", "array"}, {"description", "For face/edge: adjacent face localIds."}}}}}};
}

nlohmann::json QueryEntityInfoAction::execute(const nlohmann::json& param,
                                              const Core::ProgressCallback& progress) {
    // ── Validate parameters ──
    if(!param.contains("shapeId") || !param["shapeId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'shapeId'."}};
    }
    if(!param.contains("entityType") || !param["entityType"].is_string()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'entityType'."}};
    }
    if(!param.contains("localId") || !param["localId"].is_number()) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing 'localId'."}};
    }

    const auto shape_id = param["shapeId"].get<uint32_t>();
    const auto type_str = param["entityType"].get<std::string>();
    const auto local_id = param["localId"].get<uint32_t>();

    // Map LLM-friendly short names to internal EntityType
    std::optional<Core::EntityType> entity_type;
    if(type_str == "face") {
        entity_type = Core::EntityType::GeoFace;
    } else if(type_str == "edge") {
        entity_type = Core::EntityType::GeoEdge;
    } else if(type_str == "vertex") {
        entity_type = Core::EntityType::GeoVertex;
    } else {
        entity_type = Core::parseEntityType(type_str);
    }

    if(!entity_type.has_value()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Invalid entityType '" + type_str + "'."}};
    }

    const auto* entry = m_store.find(shape_id);
    if(!entry) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Unknown shapeId."}};
    }

    // ── Retrieve sub-shape ──
    auto sub = m_store.subShape(shape_id, *entity_type, local_id);
    if(sub.IsNull()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "localId out of range for this entityType."}};
    }

    if(progress) {
        progress(0.3, "Extracting info...");
    }

    nlohmann::json result = {{"ok", true},
                             {"action", ACTION_NAME},
                             {"shapeId", shape_id},
                             {"entityType", type_str},
                             {"localId", local_id}};

    // Bounding box
    auto bounds = computeSubShapeBounds(sub);
    if(bounds) {
        result["boundingBox"] = {{"min", bounds->first}, {"max", bounds->second}};
    }

    // ── Type-specific info + adjacency ──
    switch(*entity_type) {
    case Core::EntityType::GeoFace: {
        auto info = extractFaceInfo(local_id, TopoDS::Face(sub));
        result["surfaceType"] = info.surfaceType;
        result["center"] = info.center;
        result["normal"] = info.normal;
        result["area"] = info.area;
        if(info.axis) {
            result["axis"] = *info.axis;
        }
        if(info.radius) {
            result["radius"] = *info.radius;
        }

        // Adjacent edges (edges of this face)
        auto face_to_edge = buildFaceToEdgeAdjacency(*entry);
        if(auto it = face_to_edge.find(local_id); it != face_to_edge.end()) {
            result["adjacentEdges"] = it->second;
        } else {
            result["adjacentEdges"] = nlohmann::json::array();
        }

        // Adjacent faces (faces that share an edge with this face)
        auto edge_to_face = buildEdgeToFaceAdjacency(*entry);
        std::set<uint32_t> adj_faces;
        if(auto it = face_to_edge.find(local_id); it != face_to_edge.end()) {
            for(auto edge_id : it->second) {
                if(auto eit = edge_to_face.find(edge_id); eit != edge_to_face.end()) {
                    for(auto fid : eit->second) {
                        if(fid != local_id) {
                            adj_faces.insert(fid);
                        }
                    }
                }
            }
        }
        result["adjacentFaces"] = std::vector<uint32_t>(adj_faces.begin(), adj_faces.end());
        break;
    }

    case Core::EntityType::GeoEdge: {
        auto info = extractEdgeInfo(local_id, TopoDS::Edge(sub));
        result["curveType"] = info.curveType;
        result["start"] = info.start;
        result["end"] = info.end;
        result["length"] = info.length;
        if(info.center) {
            result["center"] = *info.center;
        }
        if(info.radius) {
            result["radius"] = *info.radius;
        }

        // Adjacent faces
        auto edge_to_face = buildEdgeToFaceAdjacency(*entry);
        if(auto it = edge_to_face.find(local_id); it != edge_to_face.end()) {
            result["adjacentFaces"] = it->second;
        } else {
            result["adjacentFaces"] = nlohmann::json::array();
        }
        break;
    }

    case Core::EntityType::GeoVertex: {
        auto info = extractVertexInfo(local_id, TopoDS::Vertex(sub));
        result["position"] = info.position;

        // Adjacent edges
        auto vtx_to_edge = buildVertexToEdgeAdjacency(*entry);
        if(auto it = vtx_to_edge.find(local_id); it != vtx_to_edge.end()) {
            result["adjacentEdges"] = it->second;
        } else {
            result["adjacentEdges"] = nlohmann::json::array();
        }
        break;
    }

    default:
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Only 'face', 'edge', and 'vertex' are supported."}};
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return result;
}

} // namespace OpenGeoLab::Geometry
