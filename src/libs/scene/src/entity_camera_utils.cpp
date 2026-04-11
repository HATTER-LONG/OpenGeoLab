/**
 * @file entity_camera_utils.cpp
 * @brief Entity-to-camera target computation for semantic camera commands
 */

#include "entity_camera_utils.hpp"

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/shape_store.hpp>
#include <opengeolab/geometry/topology_utils.hpp>

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>

#include <glm/geometric.hpp>

#include <cmath>
#include <set>

namespace OpenGeoLab::Scene {

namespace {

/// Convert a 3-element double array to glm::vec3.
glm::vec3 toVec3(const std::array<double, 3>& arr) {
    return {static_cast<float>(arr[0]), static_cast<float>(arr[1]), static_cast<float>(arr[2])};
}

/// Map LLM-friendly short names to internal EntityType.
std::optional<Core::EntityType> resolveEntityType(std::string_view type_str) {
    if(type_str == "face") {
        return Core::EntityType::GeoFace;
    }
    if(type_str == "edge") {
        return Core::EntityType::GeoEdge;
    }
    if(type_str == "vertex") {
        return Core::EntityType::GeoVertex;
    }
    return Core::parseEntityType(type_str);
}

/// Compute average face normal for a set of face localIds.
glm::vec3 averageFaceNormal(const Geometry::ShapeEntry& entry, const std::set<uint32_t>& face_ids) {
    glm::vec3 sum{0};
    for(auto fid : face_ids) {
        if(fid >= 1 && fid <= static_cast<uint32_t>(entry.faceMap.Extent())) {
            auto info =
                Geometry::extractFaceInfo(fid, TopoDS::Face(entry.faceMap(static_cast<int>(fid))));
            sum += toVec3(info.normal);
        }
    }
    const float len = glm::length(sum);
    if(len > 1.0e-6F) {
        return sum / len;
    }
    return {0.0F, 0.0F, 1.0F};
}

/// Build BoundingBox3D from an OCC sub-shape.
BoundingBox3D boundsFromShape(const TopoDS_Shape& shape) {
    BoundingBox3D bb;
    Bnd_Box occ_box;
    BRepBndLib::Add(shape, occ_box);
    if(!occ_box.IsVoid()) {
        Standard_Real xn = 0;
        Standard_Real yn = 0;
        Standard_Real zn = 0;
        Standard_Real xx = 0;
        Standard_Real yx = 0;
        Standard_Real zx = 0;
        occ_box.Get(xn, yn, zn, xx, yx, zx);
        bb.expand(
            glm::vec3{static_cast<float>(xn), static_cast<float>(yn), static_cast<float>(zn)});
        bb.expand(
            glm::vec3{static_cast<float>(xx), static_cast<float>(yx), static_cast<float>(zx)});
    }
    return bb;
}

} // anonymous namespace

std::optional<EntityCameraTarget> computeEntityCameraTarget(const Geometry::ShapeStore& store,
                                                            uint32_t shape_id,
                                                            std::string_view entity_type,
                                                            uint32_t local_id,
                                                            std::string* out_error) {
    auto parsed_type = resolveEntityType(entity_type);
    if(!parsed_type) {
        if(out_error) {
            *out_error = "Invalid entityType '" + std::string(entity_type) + "'.";
        }
        return std::nullopt;
    }

    const auto* entry = store.find(shape_id);
    if(!entry) {
        if(out_error) {
            *out_error = "Unknown shapeId.";
        }
        return std::nullopt;
    }

    auto sub = store.subShape(shape_id, *parsed_type, local_id);
    if(sub.IsNull()) {
        if(out_error) {
            *out_error = "localId out of range for this entityType.";
        }
        return std::nullopt;
    }

    EntityCameraTarget result;
    result.entityBounds = boundsFromShape(sub);

    switch(*parsed_type) {
    case Core::EntityType::GeoFace: {
        auto info = Geometry::extractFaceInfo(
            local_id, TopoDS::Face(entry->faceMap(static_cast<int>(local_id))));
        result.center = toVec3(info.center);
        result.direction = toVec3(info.normal);
        float len = glm::length(result.direction);
        if(len > 1.0e-6F) {
            result.direction /= len;
        } else {
            result.direction = {0, 0, 1};
        }
        break;
    }

    case Core::EntityType::GeoEdge: {
        auto info = Geometry::extractEdgeInfo(
            local_id, TopoDS::Edge(entry->edgeMap(static_cast<int>(local_id))));
        result.center = (toVec3(info.start) + toVec3(info.end)) * 0.5F;

        auto edge_to_face = Geometry::buildEdgeToFaceAdjacency(*entry);
        std::set<uint32_t> adj_faces;
        if(auto it = edge_to_face.find(local_id); it != edge_to_face.end()) {
            adj_faces.insert(it->second.begin(), it->second.end());
        }
        result.direction = averageFaceNormal(*entry, adj_faces);
        break;
    }

    case Core::EntityType::GeoVertex: {
        auto info = Geometry::extractVertexInfo(
            local_id, TopoDS::Vertex(entry->vertexMap(static_cast<int>(local_id))));
        result.center = toVec3(info.position);

        auto vtx_to_edge = Geometry::buildVertexToEdgeAdjacency(*entry);
        auto edge_to_face = Geometry::buildEdgeToFaceAdjacency(*entry);
        std::set<uint32_t> adj_faces;
        if(auto vit = vtx_to_edge.find(local_id); vit != vtx_to_edge.end()) {
            for(auto eid : vit->second) {
                if(auto eit = edge_to_face.find(eid); eit != edge_to_face.end()) {
                    adj_faces.insert(eit->second.begin(), eit->second.end());
                }
            }
        }
        result.direction = averageFaceNormal(*entry, adj_faces);
        break;
    }

    default:
        if(out_error) {
            *out_error = "Only 'face', 'edge', and 'vertex' are supported.";
        }
        return std::nullopt;
    }

    return result;
}

glm::vec3 chooseUpVector(const glm::vec3& direction) {
    const glm::vec3 world_y{0.0F, 1.0F, 0.0F};
    const glm::vec3 world_z{0.0F, 0.0F, 1.0F};
    if(std::abs(glm::dot(direction, world_y)) < std::abs(glm::dot(direction, world_z))) {
        return world_y;
    }
    return world_z;
}

} // namespace OpenGeoLab::Scene
