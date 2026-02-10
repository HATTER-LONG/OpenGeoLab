/**
 * @file get_part_list_action.cpp
 * @brief Implementation of get part list action
 */

#include "get_part_list_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "geometry/part_color.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {

nlohmann::json GetPartListAction::execute(const nlohmann::json& /*params*/,
                                          Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    if(progress_callback && !progress_callback(0.1, "Retrieving part list...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }

    auto document = GeometryDocumentImpl::instance();
    if(!document) {
        LOG_ERROR("GetPartListAction: No active document");
        response["success"] = false;
        response["error"] = "No active document";
        return response;
    }

    // Get all Part entities
    auto parts = document->entitiesByType(EntityType::Part);

    if(progress_callback && !progress_callback(0.3, "Processing parts...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }

    nlohmann::json parts_array = nlohmann::json::array();
    size_t processed_count = 0;

    for(const auto& part : parts) {
        if(!part) {
            continue;
        }

        nlohmann::json part_info;
        part_info["id"] = part->entityId();
        part_info["uid"] = part->entityUID();
        part_info["name"] = part->name();

        // Get color for this part based on entity ID for consistency
        PartColor color = PartColorPalette::getColorByEntityId(part->entityId());
        part_info["color"] = color.toHex();
        part_info["color_rgba"] = {color.r, color.g, color.b, color.a};

        // Count entities by type within this part
        nlohmann::json entity_counts;

        // Find all descendants of this part
        auto faces = document->findRelatedEntities(part->entityId(), EntityType::Face);
        auto edges = document->findRelatedEntities(part->entityId(), EntityType::Edge);
        auto vertices = document->findRelatedEntities(part->entityId(), EntityType::Vertex);
        auto solids = document->findRelatedEntities(part->entityId(), EntityType::Solid);
        auto shells = document->findRelatedEntities(part->entityId(), EntityType::Shell);
        auto wires = document->findRelatedEntities(part->entityId(), EntityType::Wire);

        size_t face_count = faces.size();
        size_t edge_count = edges.size();
        size_t vertex_count = vertices.size();
        size_t solid_count = solids.size();
        size_t shell_count = shells.size();
        size_t wire_count = wires.size();

        entity_counts["faces"] = face_count;
        entity_counts["edges"] = edge_count;
        entity_counts["vertices"] = vertex_count;
        entity_counts["solids"] = solid_count;
        entity_counts["shells"] = shell_count;
        entity_counts["wires"] = wire_count;
        entity_counts["total"] =
            face_count + edge_count + vertex_count + solid_count + shell_count + wire_count;

        part_info["entity_counts"] = entity_counts;

        // Collect entity IDs for each type
        nlohmann::json entity_ids;
        nlohmann::json face_ids = nlohmann::json::array();
        for(const auto& face : faces) {
            face_ids.push_back(face.m_id);
        }
        entity_ids["face_ids"] = face_ids;

        nlohmann::json edge_ids = nlohmann::json::array();
        for(const auto& edge : edges) {
            edge_ids.push_back(edge.m_id);
        }
        entity_ids["edge_ids"] = edge_ids;

        nlohmann::json vertex_ids = nlohmann::json::array();
        for(const auto& vertex : vertices) {
            vertex_ids.push_back(vertex.m_id);
        }
        entity_ids["vertex_ids"] = vertex_ids;

        part_info["entity_ids"] = entity_ids;

        // Bounding box
        auto bbox = part->boundingBox();
        if(bbox.isValid()) {
            part_info["bounding_box"] = {{"min", {bbox.m_min.x, bbox.m_min.y, bbox.m_min.z}},
                                         {"max", {bbox.m_max.x, bbox.m_max.y, bbox.m_max.z}}};
        }

        parts_array.push_back(part_info);
        ++processed_count;

        // Update progress
        if(progress_callback && !parts.empty()) {
            double progress = 0.3 + 0.6 * (static_cast<double>(processed_count) / parts.size());
            if(!progress_callback(progress, "Processing part " + std::to_string(processed_count) +
                                                "/" + std::to_string(parts.size()))) {
                response["success"] = false;
                response["error"] = "Operation cancelled";
                return response;
            }
        }
    }

    if(progress_callback) {
        progress_callback(1.0, "Part list retrieved successfully.");
    }

    LOG_DEBUG("GetPartListAction: Retrieved {} parts", parts_array.size());

    response["success"] = true;
    response["parts"] = parts_array;
    response["total_parts"] = parts_array.size();
    return response;
}

} // namespace OpenGeoLab::Geometry
