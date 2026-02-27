/**
 * @file query_mesh_info_action.cpp
 * @brief Implementation of QueryMeshInfoAction
 */

#include "query_mesh_info_action.hpp"

#include "../mesh_documentImpl.hpp"
#include "mesh/mesh_types.hpp"
#include "render/render_scene_controller.hpp"
#include "util/logger.hpp"

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

[[nodiscard]] nlohmann::json queryNode(const MeshDocumentImpl& doc, MeshNodeId uid) {
    try {
        const auto node = doc.findNodeById(uid);
        return nlohmann::json{{"type", "Node"},
                              {"uid", static_cast<uint64_t>(uid)},
                              {"position", {{"x", node.x()}, {"y", node.y()}, {"z", node.z()}}}};
    } catch(...) {
        return nlohmann::json{};
    }
}

[[nodiscard]] nlohmann::json
queryElement(const MeshDocumentImpl& doc, MeshElementUID uid, MeshElementType type) {
    try {
        const MeshElementRef ref(uid, type);
        const auto element = doc.findElementByRef(ref);

        const auto type_str = meshElementTypeToString(element.elementType());

        nlohmann::json info;
        info["type"] = type_str.value_or("Unknown");
        info["uid"] = static_cast<uint64_t>(element.elementUID());
        info["elementId"] = static_cast<uint64_t>(element.elementId());
        info["nodeCount"] = element.nodeCount();

        // Collect node IDs
        nlohmann::json node_ids = nlohmann::json::array();
        for(uint8_t i = 0; i < element.nodeCount(); ++i) {
            node_ids.push_back(static_cast<uint64_t>(element.nodeId(i)));
        }
        info["nodeIds"] = std::move(node_ids);

        // Collect node positions
        nlohmann::json nodes = nlohmann::json::array();
        for(uint8_t i = 0; i < element.nodeCount(); ++i) {
            try {
                const auto node = doc.findNodeById(element.nodeId(i));
                nodes.push_back(nlohmann::json{{"id", static_cast<uint64_t>(node.nodeId())},
                                               {"x", node.x()},
                                               {"y", node.y()},
                                               {"z", node.z()}});
            } catch(...) {
                nodes.push_back(nlohmann::json{{"id", static_cast<uint64_t>(element.nodeId(i))},
                                               {"error", "node not found"}});
            }
        }
        info["nodes"] = std::move(nodes);

        return info;
    } catch(...) {
        return nlohmann::json{};
    }
}

/**
 * @brief Query a mesh wireframe edge using its sequential edge ID.
 *
 * MeshLine pick IDs are sequential integers assigned during render building.
 * The render data stores a lookup table mapping each edge ID to its two
 * endpoint node IDs.
 *
 * @param doc Active mesh document.
 * @param edgeId Sequential edge ID from the pick buffer.
 * @return JSON with type, decoded node IDs, and positions; empty on failure.
 */
[[nodiscard]] nlohmann::json queryMeshLine(const MeshDocumentImpl& doc, uint64_t edgeId) {
    const auto& renderData = Render::RenderSceneController::instance().renderData();
    auto it = renderData.m_pickData.m_meshLineNodes.find(edgeId);
    if(it == renderData.m_pickData.m_meshLineNodes.end()) {
        return nlohmann::json{};
    }

    const auto nodeA = it->second.first;
    const auto nodeB = it->second.second;

    if(nodeA == INVALID_MESH_NODE_ID || nodeB == INVALID_MESH_NODE_ID) {
        return nlohmann::json{};
    }

    nlohmann::json info;
    info["type"] = "Line";
    info["uid"] = edgeId;
    info["nodeCount"] = 2;
    info["nodeIds"] = nlohmann::json::array({nodeA, nodeB});

    nlohmann::json nodes = nlohmann::json::array();
    for(auto nid : {nodeA, nodeB}) {
        try {
            const auto node = doc.findNodeById(nid);
            nodes.push_back(nlohmann::json{{"id", static_cast<uint64_t>(node.nodeId())},
                                           {"x", node.x()},
                                           {"y", node.y()},
                                           {"z", node.z()}});
        } catch(...) {
            nodes.push_back(
                nlohmann::json{{"id", static_cast<uint64_t>(nid)}, {"error", "node not found"}});
        }
    }
    info["nodes"] = std::move(nodes);
    return info;
}

} // namespace

nlohmann::json QueryMeshInfoAction::execute(const nlohmann::json& params,
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

        nlohmann::json info;

        if(type_str == "Node") {
            info = queryNode(*document, static_cast<MeshNodeId>(uid));
        } else if(type_str == "Line") {
            // MeshLine: first try as a regular Line element, then fall back
            // to decoding the composite node-pair UID from wireframe picking.
            info = queryElement(*document, static_cast<MeshElementUID>(uid), MeshElementType::Line);
            if(info.empty()) {
                info = queryMeshLine(*document, uid);
            }
        } else {
            // Try to parse as mesh element type
            auto elem_type_opt = meshElementTypeFromString(type_str);
            if(elem_type_opt.has_value()) {
                info = queryElement(*document, static_cast<MeshElementUID>(uid),
                                    elem_type_opt.value());
            } else {
                not_found.push_back(
                    nlohmann::json{{"type", type_str}, {"uid", uid}, {"error", "unknown type"}});
                ++processed;
                continue;
            }
        }

        if(info.empty()) {
            not_found.push_back(nlohmann::json{{"type", type_str}, {"uid", uid}});
        } else {
            results.push_back(std::move(info));
        }

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

    LOG_DEBUG("QueryMeshInfoAction: queried {}, found {}, not_found {}", total, results.size(),
              not_found.size());

    response["success"] = true;
    response["entities"] = std::move(results);
    response["not_found"] = std::move(not_found);
    response["total"] = total;
    return response;
}

} // namespace OpenGeoLab::Mesh
