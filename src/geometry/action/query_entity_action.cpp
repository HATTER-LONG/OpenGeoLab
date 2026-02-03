/**
 * @file query_entity_action.cpp
 * @brief Implementation of query entity action
 */

#include "query_entity_action.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "geometry/part_color.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {
namespace {
[[nodiscard]] nlohmann::json buildEntityInfo(const GeometryEntityPtr& entity,
                                             const GeometryDocumentImplPtr& document) {
    nlohmann::json info;
    info["id"] = entity->entityId();
    info["uid"] = entity->entityUID();
    info["type"] = GeometryEntity::entityTypeToString(entity->entityType());
    info["type_enum"] = static_cast<int>(entity->entityType());
    info["name"] = entity->name();

    // Parent IDs
    nlohmann::json parent_ids = nlohmann::json::array();
    for(const auto& parent : entity->parents()) {
        if(parent) {
            parent_ids.push_back(parent->entityId());
        }
    }
    info["parent_ids"] = parent_ids;

    // Child IDs
    nlohmann::json child_ids = nlohmann::json::array();
    for(const auto& child : entity->children()) {
        if(child) {
            child_ids.push_back(child->entityId());
        }
    }
    info["child_ids"] = child_ids;

    // Owning part
    auto owning_part = document->findOwningPart(entity->entityId());
    if(owning_part) {
        info["owning_part_id"] = owning_part->entityId();
        info["owning_part_name"] = owning_part->name();

        // Add part color
        PartColor color = PartColorPalette::getColorByEntityId(owning_part->entityId());
        info["part_color"] = color.toHex();
    } else if(entity->entityType() == EntityType::Part) {
        // Entity is itself a part
        info["owning_part_id"] = entity->entityId();
        info["owning_part_name"] = entity->name();

        PartColor color = PartColorPalette::getColorByEntityId(entity->entityId());
        info["part_color"] = color.toHex();
    }

    // Bounding box
    auto bbox = entity->boundingBox();
    if(bbox.isValid()) {
        info["bounding_box"] = {{"min", {bbox.m_min.m_x, bbox.m_min.m_y, bbox.m_min.m_z}},
                                {"max", {bbox.m_max.m_x, bbox.m_max.m_y, bbox.m_max.m_z}}};

        // Compute center and size for convenience
        auto center = bbox.center();
        auto size = bbox.size();
        info["center"] = {center.m_x, center.m_y, center.m_z};
        info["size"] = {size.m_x, size.m_y, size.m_z};
    }

    return info;
}

} // namespace

nlohmann::json QueryEntityAction::execute(const nlohmann::json& params,
                                          Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    if(!progress_callback(0.1, "Querying entity...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }
    auto document = GeometryDocumentManagerImpl::instance()->currentDocumentImplType();
    if(!document) {
        LOG_ERROR("QueryEntityAction: No active document");
        response["success"] = false;
        response["error"] = "No active document";
        return response;
    }
    if(params.contains("entity_ids") && params["entity_ids"].is_array()) {
        const auto& ids = params["entity_ids"];
        nlohmann::json entities = nlohmann::json::array();

        size_t total = ids.size();
        size_t processed = 0;

        for(const auto& id_json : ids) {
            if(!id_json.is_number_unsigned()) {
                continue;
            }

            EntityId entity_id = id_json.get<EntityId>();
            auto entity = document->findById(entity_id);

            if(entity) {
                entities.push_back(buildEntityInfo(entity, document));
            }

            ++processed;
            if(progress_callback && total > 0) {
                double progress = 0.1 + 0.8 * (static_cast<double>(processed) / total);
                if(!progress_callback(progress, "Querying " + std::to_string(processed) + "/" +
                                                    std::to_string(total))) {
                    response["success"] = false;
                    response["error"] = "Operation cancelled";
                    return response;
                }
            }
        }
        if(progress_callback) {
            progress_callback(1.0, "Query completed.");
        }

        response["success"] = true;
        response["entities"] = entities;
        response["queried_count"] = entities.size();
        return response;
    }

    // Single entity query
    if(!params.contains("entity_id")) {
        LOG_ERROR("QueryEntityAction: Missing 'entity_id' or 'entity_ids' parameter");
        response["success"] = false;
        response["error"] = "Missing 'entity_id' or 'entity_ids' parameter";
        return response;
    }

    EntityId entity_id = params["entity_id"].get<EntityId>();
    auto entity = document->findById(entity_id);

    if(!entity) {
        LOG_WARN("QueryEntityAction: Entity {} not found", entity_id);
        response["success"] = false;
        response["error"] = "Entity not found";
        response["entity_id"] = entity_id;
        return response;
    }

    if(progress_callback) {
        progress_callback(1.0, "Query completed.");
    }

    LOG_DEBUG("QueryEntityAction: Queried entity {} ({})", entity_id, entity->typeName());

    response["success"] = true;
    response["entity"] = buildEntityInfo(entity, document);
    return response;
}
} // namespace OpenGeoLab::Geometry