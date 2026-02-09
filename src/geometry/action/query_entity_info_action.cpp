/**
 * @file query_entity_info_action.cpp
 * @brief Implementation of QueryEntityInfoAction
 */

#include "query_entity_info_action.hpp"

#include "../geometry_document_managerImpl.hpp"
#include "geometry/geometry_types.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {

namespace {
[[nodiscard]] nlohmann::json entityKeyToJson(const EntityKey& key) {
    return nlohmann::json{{"id", key.m_id},
                          {"uid", static_cast<uint64_t>(key.m_uid)},
                          {"type", entityTypeToString(key.m_type)}};
}

[[nodiscard]] nlohmann::json entityKeyArrayToJson(const std::vector<EntityKey>& keys) {
    nlohmann::json arr = nlohmann::json::array();
    for(const auto& k : keys) {
        arr.push_back(entityKeyToJson(k));
    }
    return arr;
}

[[nodiscard]] nlohmann::json bboxToJson(const BoundingBox3D& bbox) {
    return nlohmann::json{{"min", {bbox.m_min.x, bbox.m_min.y, bbox.m_min.z}},
                          {"max", {bbox.m_max.x, bbox.m_max.y, bbox.m_max.z}}};
}

[[nodiscard]] bool validateEntityHandle(const nlohmann::json& j, std::string& error) {
    if(!j.is_object()) {
        error = "Each entity handle must be an object";
        return false;
    }
    if(!j.contains("uid") || !j["uid"].is_number_integer()) {
        error = "Entity handle requires integer field 'uid'";
        return false;
    }
    if(!j.contains("type") || !j["type"].is_string()) {
        error = "Entity handle requires string field 'type'";
        return false;
    }
    return true;
}
} // namespace

nlohmann::json QueryEntityInfoAction::execute(const nlohmann::json& params,
                                              Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    if(progress_callback && !progress_callback(0.05, "Preparing query...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }

    auto document = GeometryDocumentManagerImpl::instance()->currentDocumentImplType();
    if(!document) {
        response["success"] = false;
        response["error"] = "No active document";
        return response;
    }

    if(!params.is_object()) {
        response["success"] = false;
        response["error"] = "Invalid params: expected JSON object";
        return response;
    }

    const auto entities_it = params.find("entities");
    if(entities_it == params.end() || !entities_it->is_array()) {
        response["success"] = false;
        response["error"] = "Missing or invalid 'entities' array";
        return response;
    }

    const auto& handles = *entities_it;
    nlohmann::json results = nlohmann::json::array();
    nlohmann::json not_found = nlohmann::json::array();

    const size_t total = handles.size();
    size_t processed = 0;

    for(const auto& h : handles) {
        std::string err;
        if(!validateEntityHandle(h, err)) {
            response["success"] = false;
            response["error"] = err;
            return response;
        }

        const auto uid = static_cast<EntityUID>(h["uid"].get<uint64_t>());
        const auto type_str = h["type"].get<std::string>();

        EntityType type = EntityType::None;
        try {
            type = entityTypeFromString(type_str);
        } catch(const std::exception& e) {
            response["success"] = false;
            response["error"] = std::string("Invalid entity type: ") + e.what();
            return response;
        }

        const auto entity = document->findImplByUIDAndType(uid, type);
        if(!entity) {
            not_found.push_back(nlohmann::json{{"type", type_str}, {"uid", h["uid"]}});
            ++processed;
            continue;
        }

        nlohmann::json info;
        info["type"] = entityTypeToString(entity->entityType());
        info["uid"] = static_cast<uint64_t>(entity->entityUID());
        info["id"] = entity->entityId();
        info["name"] = entity->name();

        const auto bbox = entity->boundingBox();
        if(bbox.isValid()) {
            info["bounding_box"] = bboxToJson(bbox);
        }

        // Relationships / related entities snapshot
        nlohmann::json related;
        related["parts"] = entityKeyArrayToJson(
            document->findRelatedEntities(entity->entityId(), EntityType::Part));
        related["solids"] = entityKeyArrayToJson(
            document->findRelatedEntities(entity->entityId(), EntityType::Solid));
        related["wires"] = entityKeyArrayToJson(
            document->findRelatedEntities(entity->entityId(), EntityType::Wire));
        related["faces"] = entityKeyArrayToJson(
            document->findRelatedEntities(entity->entityId(), EntityType::Face));
        related["edges"] = entityKeyArrayToJson(
            document->findRelatedEntities(entity->entityId(), EntityType::Edge));
        related["vertices"] = entityKeyArrayToJson(
            document->findRelatedEntities(entity->entityId(), EntityType::Vertex));
        related["shells"] = entityKeyArrayToJson(
            document->findRelatedEntities(entity->entityId(), EntityType::Shell));
        info["related"] = std::move(related);

        results.push_back(std::move(info));

        ++processed;
        if(progress_callback && total > 0) {
            const double progress = 0.1 + 0.85 * (static_cast<double>(processed) / total);
            if(!progress_callback(progress, "Querying entity " + std::to_string(processed) + "/" +
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

    LOG_DEBUG("QueryEntityInfoAction: queried {}, found {}, not_found {}", total, results.size(),
              not_found.size());

    response["success"] = true;
    response["entities"] = std::move(results);
    response["not_found"] = std::move(not_found);
    response["total"] = total;
    return response;
}

} // namespace OpenGeoLab::Geometry
