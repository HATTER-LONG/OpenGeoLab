/// @file query_mesh_action.cpp
/// @brief Returns detailed information about a mesh entry.

#include <opengeolab/mesh/query_mesh_action.hpp>

#include <opengeolab/mesh/mesh_store.hpp>
#include <opengeolab/mesh/mesh_types.hpp>

#include <opengeolab/core/logger.hpp>

#include <algorithm>
#include <limits>

namespace OpenGeoLab::Mesh {

QueryMeshAction::QueryMeshAction(MeshStore& store) : m_store(store) {}
QueryMeshAction::~QueryMeshAction() = default;

nlohmann::json QueryMeshAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Query detailed information about a mesh."},
        {"params",
         {{"meshId",
           {{"type", "integer"}, {"required", true}, {"description", "Mesh ID to query"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success"}}},
          {"meshId", {{"type", "integer"}, {"description", "Mesh ID"}}},
          {"name", {{"type", "string"}, {"description", "Mesh name"}}},
          {"sourceShapeId",
           {{"type", "integer"}, {"description", "Source shape ID (null if not set)"}}},
          {"nodeCount", {{"type", "integer"}, {"description", "Number of mesh nodes"}}},
          {"elementSummary", {{"type", "object"}, {"description", "Element counts by type name"}}},
          {"boundingBox",
           {{"type", "object"}, {"description", "Axis-aligned bounding box {min, max}"}}}}}};
}

nlohmann::json QueryMeshAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    if(!param.contains("meshId") || !param["meshId"].is_number_integer()) {
        return {{"ok", false}, {"summary", "Missing or invalid 'meshId' parameter"}};
    }
    const auto mesh_id = param["meshId"].get<uint32_t>();

    const auto entry = m_store.find(mesh_id);
    if(!entry) {
        return {{"ok", false}, {"summary", "Mesh ID " + std::to_string(mesh_id) + " not found"}};
    }

    if(progress) {
        progress(0.3, "Collecting mesh info...");
    }

    // Element summary: count per element type
    nlohmann::json element_summary = nlohmann::json::object();

    auto add_block_summary = [&](const std::vector<ElementBlock>& blocks) {
        for(const auto& block : blocks) {
            auto type_name = std::to_string(static_cast<int>(block.type));
            if(element_summary.contains(type_name)) {
                element_summary[type_name] =
                    element_summary[type_name].get<size_t>() + block.elementCount();
            } else {
                element_summary[type_name] = block.elementCount();
            }
        }
    };

    add_block_summary(entry->lineBlocks);
    add_block_summary(entry->surfaceBlocks);
    add_block_summary(entry->volumeBlocks);

    // Bounding box
    constexpr auto fmax = std::numeric_limits<double>::max();
    constexpr auto fmin = std::numeric_limits<double>::lowest();
    double bmin[3] = {fmax, fmax, fmax};
    double bmax[3] = {fmin, fmin, fmin};

    for(size_t i = 0; i < entry->nodes.count(); ++i) {
        auto pos = entry->nodes.position(static_cast<uint32_t>(i + 1));
        for(int j = 0; j < 3; ++j) {
            bmin[j] = std::min(bmin[j], pos[j]);
            bmax[j] = std::max(bmax[j], pos[j]);
        }
    }

    nlohmann::json bbox;
    if(entry->nodes.count() > 0) {
        bbox = {{"min", {bmin[0], bmin[1], bmin[2]}}, {"max", {bmax[0], bmax[1], bmax[2]}}};
    } else {
        bbox = nullptr;
    }

    if(progress) {
        progress(1.0, "Done");
    }

    nlohmann::json result = {{"ok", true},
                             {"action", ACTION_NAME},
                             {"meshId", mesh_id},
                             {"name", entry->name},
                             {"nodeCount", entry->nodeCount()},
                             {"elementCount", entry->elementCount()},
                             {"elementSummary", element_summary},
                             {"boundingBox", bbox}};

    if(entry->sourceShapeId.has_value()) {
        result["sourceShapeId"] = entry->sourceShapeId.value();
    } else {
        result["sourceShapeId"] = nullptr;
    }

    return result;
}

} // namespace OpenGeoLab::Mesh
