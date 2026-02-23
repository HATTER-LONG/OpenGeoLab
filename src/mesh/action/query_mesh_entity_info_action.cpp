/**
 * @file query_mesh_entity_info_action.cpp
 * @brief Implementation of QueryMeshEntityInfoAction
 */

#include "query_mesh_entity_info_action.hpp"

#include "../mesh_documentImpl.hpp"
#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"
#include "render/render_types.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Mesh {

namespace {

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

[[nodiscard]] nlohmann::json nodeToJson(const MeshNode& node, const MeshDocumentImpl& doc) {
    nlohmann::json info;
    info["type"] = "MeshNode";
    info["nodeId"] = node.nodeId();
    info["position"] = {node.x(), node.y(), node.z()};

    // Adjacent elements (using reverse index)
    auto elem_ids = doc.findElementsByNodeId(node.nodeId());
    nlohmann::json adj_elements = nlohmann::json::array();
    for(const auto eid : elem_ids) {
        const auto* elem = doc.findElementById(eid);
        if(elem) {
            adj_elements.push_back({{"elementId", eid},
                                    {"elementUID", elem->elementUID()},
                                    {"elementType", meshElementTypeToString(elem->elementType())}});
        }
    }
    info["adjacentElements"] = std::move(adj_elements);

    // Adjacent nodes (connected via shared elements)
    auto adj_node_ids = doc.findAdjacentNodes(node.nodeId());
    nlohmann::json adj_nodes = nlohmann::json::array();
    for(const auto nid : adj_node_ids) {
        const auto* adj_node = doc.findNodeById(nid);
        if(adj_node) {
            adj_nodes.push_back(
                {{"nodeId", nid}, {"position", {adj_node->x(), adj_node->y(), adj_node->z()}}});
        }
    }
    info["adjacentNodes"] = std::move(adj_nodes);

    return info;
}

[[nodiscard]] nlohmann::json elementToJson(const MeshElement& elem, const MeshDocumentImpl& doc) {
    nlohmann::json info;
    info["type"] = "MeshElement";
    info["elementId"] = elem.elementId();
    info["elementUID"] = elem.elementUID();
    info["elementType"] = meshElementTypeToString(elem.elementType());
    info["nodeCount"] = elem.nodeCount();

    // Node positions
    nlohmann::json nodes = nlohmann::json::array();
    const uint8_t nc = elem.nodeCount();
    for(uint8_t i = 0; i < nc; ++i) {
        const MeshNodeId nid = elem.nodeId(i);
        const auto* node = doc.findNodeById(nid);
        if(node) {
            nodes.push_back({{"nodeId", nid}, {"position", {node->x(), node->y(), node->z()}}});
        } else {
            nodes.push_back({{"nodeId", nid}, {"position", nullptr}});
        }
    }
    info["nodes"] = std::move(nodes);

    // Adjacent elements (sharing at least one node)
    auto adj_elem_ids = doc.findAdjacentElements(elem.elementId());
    nlohmann::json adj_elements = nlohmann::json::array();
    for(const auto eid : adj_elem_ids) {
        const auto* adj_elem = doc.findElementById(eid);
        if(adj_elem) {
            adj_elements.push_back(
                {{"elementId", eid},
                 {"elementUID", adj_elem->elementUID()},
                 {"elementType", meshElementTypeToString(adj_elem->elementType())}});
        }
    }
    info["adjacentElements"] = std::move(adj_elements);

    return info;
}

} // namespace

nlohmann::json QueryMeshEntityInfoAction::execute(const nlohmann::json& params,
                                                  Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    if(progress_callback && !progress_callback(0.05, "Preparing mesh query...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }

    auto document = MeshDocumentImpl::instance();
    if(!document) {
        response["success"] = false;
        response["error"] = "No active mesh document";
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

        const auto uid = h["uid"].get<uint64_t>();
        const auto type_str = h["type"].get<std::string>();
        const auto render_type = Render::renderEntityTypeFromString(type_str);

        if(render_type == Render::RenderEntityType::MeshNode) {
            // Look up node by UID (nodeId == uid for mesh nodes)
            const auto node_id = static_cast<MeshNodeId>(uid);
            const auto* node = document->findNodeById(node_id);
            if(node) {
                results.push_back(nodeToJson(*node, *document));
            } else {
                not_found.push_back({{"uid", uid}, {"type", type_str}});
            }
        } else if(render_type == Render::RenderEntityType::MeshElement) {
            // Look up element by UID
            const auto elem_uid = static_cast<MeshElementUID>(uid);
            const auto* elem = document->findElementByUID(elem_uid);
            if(elem) {
                results.push_back(elementToJson(*elem, *document));
            } else {
                not_found.push_back({{"uid", uid}, {"type", type_str}});
            }
        } else {
            not_found.push_back(
                {{"uid", uid}, {"type", type_str}, {"reason", "Unknown mesh entity type"}});
        }

        ++processed;
        if(progress_callback && total > 0) {
            const double progress = 0.1 + 0.85 * (static_cast<double>(processed) / total);
            if(!progress_callback(progress, "Querying mesh entity " + std::to_string(processed) +
                                                "/" + std::to_string(total))) {
                response["success"] = false;
                response["error"] = "Operation cancelled";
                return response;
            }
        }
    }

    if(progress_callback) {
        progress_callback(1.0, "Mesh query completed.");
    }

    LOG_DEBUG("QueryMeshEntityInfoAction: queried {}, found {}, not_found {}", total,
              results.size(), not_found.size());

    response["success"] = true;
    response["entities"] = std::move(results);
    response["not_found"] = std::move(not_found);
    response["total"] = total;
    return response;
}

} // namespace OpenGeoLab::Mesh
