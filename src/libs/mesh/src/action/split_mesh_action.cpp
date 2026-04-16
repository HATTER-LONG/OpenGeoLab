/**
 * @file split_mesh_action.cpp
 * @brief SplitMeshAction implementation
 */

#include "split_mesh_action.hpp"

#include <opengeolab/mesh/mesh_node.hpp>
#include <opengeolab/mesh/mesh_split_algorithm.hpp>
#include <opengeolab/mesh/mesh_store.hpp>
#include <opengeolab/mesh/split_mode.hpp>
#include <opengeolab/mesh/split_result.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Mesh {

namespace {

nlohmann::json makeFailure(std::string_view summary) {
    return {
        {"ok", false},
        {"action", SplitMeshAction::ACTION_NAME},
        {"summary", std::string(summary)},
    };
}

std::optional<uint32_t> parseUint32(const nlohmann::json& value) {
    if(value.is_number_unsigned()) {
        return value.get<uint32_t>();
    }

    if(!value.is_number_integer()) {
        return std::nullopt;
    }

    const auto parsed_value = value.get<int64_t>();
    if(parsed_value < 0 ||
       parsed_value > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<uint32_t>(parsed_value);
}

SplitMode parseSplitMode(const nlohmann::json& mode_value) {
    if(mode_value.is_number_unsigned()) {
        return static_cast<SplitMode>(mode_value.get<uint8_t>());
    }
    if(mode_value.is_number_integer()) {
        const auto value = mode_value.get<int64_t>();
        if(value >= 0 && value <= 255) {
            return static_cast<SplitMode>(static_cast<uint8_t>(value));
        }
    }

    const auto mode_str = mode_value.get<std::string>();
    if(mode_str.empty() || mode_str == "auto") {
        return SplitMode::Auto;
    }
    if(mode_str == "tria_one_quad_three") {
        return SplitMode::TriaOneQuadThree;
    }
    if(mode_str == "tria_one_quad_two") {
        return SplitMode::TriaOneQuadTwo;
    }
    if(mode_str == "tria_three_quad_two") {
        return SplitMode::TriaThreeQuadTwo;
    }
    if(mode_str == "tria_four") {
        return SplitMode::TriaFour;
    }
    if(mode_str == "quad_three") {
        return SplitMode::QuadThree;
    }
    if(mode_str == "tria_three") {
        return SplitMode::TriaThree;
    }
    throw std::invalid_argument("Unknown split mode: " + mode_str);
}

void applySplitResult(MeshEntry& entry, const SplitResult& result) {
    for(const auto& new_node : result.newNodes) {
        entry.nodes.push_back(
            MeshNode{{static_cast<float>(new_node.x), static_cast<float>(new_node.y),
                      static_cast<float>(new_node.z)}});
    }

    std::vector<std::size_t> sorted_indices(result.replacements.size());
    for(std::size_t index = 0; index < sorted_indices.size(); ++index) {
        sorted_indices[index] = index;
    }
    std::sort(sorted_indices.begin(), sorted_indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        return result.replacements[lhs].originalIndex > result.replacements[rhs].originalIndex;
    });

    for(const auto replacement_index : sorted_indices) {
        const auto& replacement = result.replacements[replacement_index];
        auto position =
            entry.elements.begin() + static_cast<std::ptrdiff_t>(replacement.originalIndex);
        position = entry.elements.erase(position);
        entry.elements.insert(position, replacement.newElements.begin(),
                              replacement.newElements.end());
    }
}

} // namespace

SplitMeshAction::SplitMeshAction(MeshStore& mesh_store) : m_meshStore(mesh_store) {}

SplitMeshAction::~SplitMeshAction() = default;

nlohmann::json SplitMeshAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Split mesh elements by subdividing along selected edges or nodes."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", true},
            {"description", "Shape identifier of the target mesh."}}},
          {"selections",
           {{"type", "array"},
            {"required", true},
            {"description", "Array of {type, localId} — type is \"edge\" or \"node\"."}}},
          {"mode",
           {{"type", "string|integer"},
            {"required", false},
            {"description",
             "Split mode: string name (tria_one_quad_three, etc.) or numeric bitmask "
             "combining one quad mode (1/2/4) with one triangle mode (8/16). "
             "Use 32 for node split (tria_three). Default: auto (0)."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"shapeId",
           {{"type", "integer"}, {"description", "Shape identifier of the modified mesh."}}},
          {"summary",
           {{"type", "string"},
            {"description", "Human-readable summary of the split operation."}}}}}};
}

nlohmann::json SplitMeshAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    if(!param.contains("shapeId")) {
        return makeFailure("Missing or invalid 'shapeId' parameter.");
    }

    const auto shape_id = parseUint32(param["shapeId"]);
    if(!shape_id.has_value()) {
        return makeFailure("Missing or invalid 'shapeId' parameter.");
    }

    if(!param.contains("selections") || !param["selections"].is_array()) {
        return makeFailure("Missing or invalid 'selections' parameter.");
    }

    SplitMode mode{};
    try {
        const auto& mode_value = param.contains("mode") ? param["mode"] : nlohmann::json("auto");
        mode = parseSplitMode(mode_value);
    } catch(const std::invalid_argument& ex) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"summary", ex.what()}};
    }

    std::vector<uint32_t> selected_edges;
    std::vector<uint32_t> selected_nodes;
    for(const auto& selection : param["selections"]) {
        if(!selection.is_object()) {
            continue;
        }

        const auto selection_type = selection.value("type", std::string{});
        if(!selection.contains("localId")) {
            continue;
        }

        const auto local_id = parseUint32(selection["localId"]);
        if(!local_id.has_value() || *local_id == 0) {
            continue;
        }

        if(selection_type == "edge") {
            selected_edges.push_back(*local_id);
        } else if(selection_type == "node") {
            selected_nodes.push_back(*local_id);
        }
    }

    if(selected_edges.empty() && selected_nodes.empty()) {
        return makeFailure("No valid selections provided.");
    }

    const auto* entry_ptr = m_meshStore.find(*shape_id);
    if(entry_ptr == nullptr) {
        return makeFailure("Mesh not found for shapeId " + std::to_string(*shape_id) + ".");
    }

    const auto* topology_ptr = m_meshStore.getTopology(*shape_id);
    if(topology_ptr == nullptr) {
        return makeFailure("Topology not available for shapeId " + std::to_string(*shape_id) + ".");
    }

    const auto entry = *entry_ptr;
    const auto topology = *topology_ptr;

    const MeshSplitAlgorithm algorithm;
    const auto result = algorithm.compute(entry, topology, selected_edges, selected_nodes, mode);

    if(result.replacements.empty()) {
        return {{"ok", true},
                {"action", ACTION_NAME},
                {"shapeId", *shape_id},
                {"summary",
                 "No elements were split (selections did not match any splittable elements)."}};
    }

    std::size_t new_element_count = 0;
    for(const auto& replacement : result.replacements) {
        new_element_count += replacement.newElements.size();
    }

    m_meshStore.modifyMesh(*shape_id, [&result](MeshEntry& mutable_entry) {
        applySplitResult(mutable_entry, result);
    });

    if(progress) {
        progress(1.0, "Done");
    }

    const auto summary = "Split completed: " + std::to_string(result.replacements.size()) +
                         " elements replaced, " + std::to_string(result.newNodes.size()) +
                         " new nodes, " + std::to_string(new_element_count) + " new elements.";

    return {
        {"ok", true},
        {"action", ACTION_NAME},
        {"shapeId", *shape_id},
        {"summary", summary},
    };
}

} // namespace OpenGeoLab::Mesh
