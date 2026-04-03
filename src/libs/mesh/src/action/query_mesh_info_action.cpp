/**
 * @file query_mesh_info_action.cpp
 * @brief QueryMeshInfoAction implementation
 */

#include "query_mesh_info_action.hpp"

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/mesh/mesh_element_type.hpp>
#include <opengeolab/mesh/mesh_store.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace OpenGeoLab::Mesh {

namespace {

std::optional<OpenGeoLab::Core::EntityType> parseMeshEntityType(const std::string& type_name) {
    if(type_name == "MeshNode") {
        return OpenGeoLab::Core::EntityType::MeshNode;
    }
    if(type_name == "MeshEdge") {
        return OpenGeoLab::Core::EntityType::MeshEdge;
    }
    if(type_name == "MeshElement") {
        return OpenGeoLab::Core::EntityType::MeshElement;
    }
    return std::nullopt;
}

std::string_view meshElementTypeName(const MeshElementType type) {
    switch(type) {
    case MeshElementType::Triangle:
        return "Triangle";
    case MeshElementType::Quad:
        return "Quad";
    case MeshElementType::Tetra:
        return "Tetra";
    case MeshElementType::Hexa:
        return "Hexa";
    case MeshElementType::Prism:
        return "Prism";
    case MeshElementType::Pyramid:
        return "Pyramid";
    }
    return "Unknown";
}

} // namespace

QueryMeshInfoAction::QueryMeshInfoAction(const MeshStore& mesh_store) : m_meshStore(mesh_store) {}

QueryMeshInfoAction::~QueryMeshInfoAction() = default;

nlohmann::json QueryMeshInfoAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Query mesh node, edge, and element information for stored meshes."},
        {"params",
         {{"entities",
           {{"type", "array"},
            {"required", true},
            {"description",
             "Array of {shapeId, type, localId} — MeshNode, MeshEdge, or MeshElement."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"entities",
           {{"type", "array"},
            {"description", "Per-entity mesh information or acknowledgment results."}}}}}};
}

nlohmann::json QueryMeshInfoAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto entities = param.value("entities", nlohmann::json::array());
    nlohmann::json results = nlohmann::json::array();

    for(const auto& entity_json : entities) {
        const auto shape_id = entity_json.value("shapeId", static_cast<uint32_t>(0));
        const auto local_id = entity_json.value("localId", static_cast<uint32_t>(0));
        const auto type_name = entity_json.value("type", std::string{});

        nlohmann::json entity_result = {
            {"shapeId", shape_id},
            {"type", type_name},
            {"localId", local_id},
        };

        const auto entity_type = parseMeshEntityType(type_name);
        if(!entity_type.has_value()) {
            entity_result["ok"] = false;
            entity_result["summary"] = "Unknown mesh entity type";
            results.push_back(std::move(entity_result));
            continue;
        }

        const auto* entry = m_meshStore.find(shape_id);
        if(entry == nullptr) {
            entity_result["ok"] = false;
            entity_result["summary"] = "Unknown shapeId";
            results.push_back(std::move(entity_result));
            continue;
        }

        switch(*entity_type) {
        case Core::EntityType::MeshNode:
            if(local_id == 0 || local_id > entry->nodes.size()) {
                entity_result["ok"] = false;
                entity_result["summary"] = "Unknown mesh node localId";
                break;
            }
            entity_result["ok"] = true;
            entity_result["position"] = {entry->nodes[local_id - 1].position[0],
                                         entry->nodes[local_id - 1].position[1],
                                         entry->nodes[local_id - 1].position[2]};
            break;
        case Core::EntityType::MeshElement:
            if(local_id == 0 || local_id > entry->elements.size()) {
                entity_result["ok"] = false;
                entity_result["summary"] = "Unknown mesh element localId";
                break;
            }
            entity_result["ok"] = true;
            entity_result["elementType"] = meshElementTypeName(entry->elements[local_id - 1].type);
            entity_result["nodeLocalIds"] = nlohmann::json::array();
            for(uint8_t i = 0; i < nodeCount(entry->elements[local_id - 1].type); ++i) {
                entity_result["nodeLocalIds"].push_back(
                    entry->elements[local_id - 1].nodeLocalIds[i]);
            }
            break;
        case Core::EntityType::MeshEdge:
            entity_result["ok"] = true;
            entity_result["summary"] = "Mesh edges are derived at render time only.";
            break;
        default:
            entity_result["ok"] = false;
            entity_result["summary"] = "Unsupported mesh entity type";
            break;
        }

        results.push_back(std::move(entity_result));
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"entities", std::move(results)}};
}

} // namespace OpenGeoLab::Mesh
