/**
 * @file smooth_mesh_action.cpp
 * @brief Smooth selected mesh nodes or elements using Laplacian/Taubin relaxation.
 */

#include "smooth_mesh_action.hpp"

#include "../mesh_documentImpl.hpp"
#include "geometry/geometry_types.hpp"
#include "mesh/mesh_types.hpp"
#include "util/point_vector3d.hpp"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace OpenGeoLab::Mesh {
namespace {

struct SmoothSettings {
    int m_iterations{5};
    double m_factor{0.35};
    bool m_preserveBoundaries{true};
    std::string m_method{"laplacian"};
};

struct TargetSelection {
    std::unordered_set<MeshNodeId> m_nodeIds;
    MeshElementRefSet m_lineRefs;
    MeshElementRefSet m_elementRefs;
    std::unordered_set<uint64_t> m_partUids;
};

struct EdgeTableView {
    const int (*m_edges)[2]{nullptr};
    size_t m_count{0};
};

[[nodiscard]] uint64_t makeEdgeKey(MeshNodeId a, MeshNodeId b) {
    const auto lo = std::min(a, b);
    const auto hi = std::max(a, b);
    return (lo << 32u) | (hi & 0xFFFFFFFFu);
}

[[nodiscard]] EdgeTableView edgeTableForType(MeshElementType type) {
    static constexpr int triangle_edges[][2] = {{0, 1}, {1, 2}, {2, 0}};
    static constexpr int quad4_edges[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    static constexpr int tetra4_edges[][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    static constexpr int hexa8_edges[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                             {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    static constexpr int prism6_edges[][2] = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5},
                                              {5, 3}, {0, 3}, {1, 4}, {2, 5}};
    static constexpr int pyramid5_edges[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0},
                                                {0, 4}, {1, 4}, {2, 4}, {3, 4}};

    switch(type) {
    case MeshElementType::Triangle:
        return {triangle_edges, 3};
    case MeshElementType::Quad4:
        return {quad4_edges, 4};
    case MeshElementType::Tetra4:
        return {tetra4_edges, 6};
    case MeshElementType::Hexa8:
        return {hexa8_edges, 12};
    case MeshElementType::Prism6:
        return {prism6_edges, 9};
    case MeshElementType::Pyramid5:
        return {pyramid5_edges, 8};
    default:
        return {};
    }
}

[[nodiscard]] bool isSurfaceOrVolume(MeshElementType type) {
    return type == MeshElementType::Triangle || type == MeshElementType::Quad4 ||
           type == MeshElementType::Tetra4 || type == MeshElementType::Hexa8 ||
           type == MeshElementType::Prism6 || type == MeshElementType::Pyramid5;
}

[[nodiscard]] std::optional<SmoothSettings> parseSettings(const nlohmann::json& params,
                                                          std::string& error) {
    SmoothSettings settings;
    settings.m_iterations = params.value("iterations", 5);
    if(settings.m_iterations < 1 || settings.m_iterations > 100) {
        error = "Invalid 'iterations': must be in [1, 100]";
        return std::nullopt;
    }

    settings.m_factor = params.value("factor", 0.35);
    if(!(settings.m_factor > 0.0) || settings.m_factor > 1.0) {
        error = "Invalid 'factor': must be in (0, 1]";
        return std::nullopt;
    }

    settings.m_method = params.value("method", std::string{"laplacian"});
    if(settings.m_method != "laplacian" && settings.m_method != "taubin") {
        error = "Invalid 'method': must be 'laplacian' or 'taubin'";
        return std::nullopt;
    }

    settings.m_preserveBoundaries = params.value("preserveBoundaries", true);
    return settings;
}

[[nodiscard]] std::optional<TargetSelection> parseTargets(const nlohmann::json& params,
                                                          std::string& error) {
    const auto entities_it = params.find("entities");
    if(entities_it == params.end() || !entities_it->is_array()) {
        error = "Missing or invalid 'entities' array";
        return std::nullopt;
    }

    TargetSelection selection;
    for(const auto& handle : *entities_it) {
        if(!handle.is_object() || !handle.contains("uid") || !handle.contains("type") ||
           !handle["uid"].is_number_unsigned() || !handle["type"].is_string()) {
            error = "Each smoothing target must contain unsigned uid and string type";
            return std::nullopt;
        }

        const auto uid = handle["uid"].get<uint64_t>();
        const auto type_name = handle["type"].get<std::string>();

        if(const auto geometry_type = Geometry::entityTypeFromString(type_name);
           geometry_type.has_value()) {
            if(geometry_type.value() != Geometry::EntityType::Part) {
                error = "Smoothing only accepts mesh entities or Part targets";
                return std::nullopt;
            }
            selection.m_partUids.emplace(uid);
            continue;
        }

        const auto mesh_type = meshElementTypeFromString(type_name);
        if(!mesh_type.has_value()) {
            error = "Unsupported smoothing target type";
            return std::nullopt;
        }

        switch(mesh_type.value()) {
        case MeshElementType::Node:
            selection.m_nodeIds.emplace(static_cast<MeshNodeId>(uid));
            break;
        case MeshElementType::Line:
            selection.m_lineRefs.emplace(
                MeshElementRef(static_cast<MeshElementUID>(uid), MeshElementType::Line));
            break;
        case MeshElementType::Triangle:
        case MeshElementType::Quad4:
        case MeshElementType::Tetra4:
        case MeshElementType::Hexa8:
        case MeshElementType::Prism6:
        case MeshElementType::Pyramid5:
            selection.m_elementRefs.emplace(
                MeshElementRef(static_cast<MeshElementUID>(uid), mesh_type.value()));
            break;
        default:
            error = "Unsupported smoothing target type";
            return std::nullopt;
        }
    }

    if(selection.m_nodeIds.empty() && selection.m_lineRefs.empty() &&
       selection.m_elementRefs.empty() && selection.m_partUids.empty()) {
        error = "No smoothing targets provided";
        return std::nullopt;
    }

    return selection;
}

void collectPartElements(const std::vector<MeshElement>& elements,
                         uint64_t part_uid,
                         MeshElementRefSet& element_refs,
                         std::unordered_set<MeshNodeId>& target_nodes) {
    for(const auto& element : elements) {
        if(!element.isValid() || element.partUid() != part_uid ||
           !isSurfaceOrVolume(element.elementType())) {
            continue;
        }
        element_refs.emplace(element.elementRef());
        for(uint8_t index = 0; index < element.nodeCount(); ++index) {
            const auto node_id = element.nodeId(index);
            if(node_id != INVALID_MESH_NODE_ID) {
                target_nodes.emplace(node_id);
            }
        }
    }
}

void collectLineNodes(const std::vector<MeshElement>& elements,
                      const MeshElementRefSet& line_refs,
                      std::unordered_set<MeshNodeId>& node_ids) {
    if(line_refs.empty()) {
        return;
    }

    for(const auto& element : elements) {
        if(!element.isValid() || element.elementType() != MeshElementType::Line) {
            continue;
        }
        if(!line_refs.contains(element.elementRef())) {
            continue;
        }
        node_ids.emplace(element.nodeId(0));
        node_ids.emplace(element.nodeId(1));
    }
}

[[nodiscard]] std::unordered_map<MeshElementRef, MeshElement, MeshElementRefHash>
buildElementIndex(const std::vector<MeshElement>& elements) {
    std::unordered_map<MeshElementRef, MeshElement, MeshElementRefHash> index;
    index.reserve(elements.size());
    for(const auto& element : elements) {
        if(element.isValid()) {
            index.emplace(element.elementRef(), element);
        }
    }
    return index;
}

[[nodiscard]] std::unordered_map<MeshNodeId, MeshNode>
buildNodeIndex(const std::vector<MeshNode>& nodes) {
    std::unordered_map<MeshNodeId, MeshNode> index;
    index.reserve(nodes.size());
    for(const auto& node : nodes) {
        if(node.nodeId() != INVALID_MESH_NODE_ID) {
            index.emplace(node.nodeId(), node);
        }
    }
    return index;
}

void expandSelectedElements(const MeshDocumentImpl& document,
                            const std::unordered_set<MeshNodeId>& explicit_node_ids,
                            const MeshElementRefSet& explicit_line_refs,
                            MeshElementRefSet& expanded_element_refs) {
    for(const auto node_id : explicit_node_ids) {
        for(const auto& element_ref : document.findElementsByNodeId(node_id)) {
            expanded_element_refs.emplace(element_ref);
        }
    }

    for(const auto& line_ref : explicit_line_refs) {
        for(const auto& element_ref : document.findElementsByLineRef(line_ref)) {
            expanded_element_refs.emplace(element_ref);
        }
    }
}

void collectTargetNodesFromElements(
    const std::unordered_map<MeshElementRef, MeshElement, MeshElementRefHash>& element_index,
    const MeshElementRefSet& element_refs,
    std::unordered_set<MeshNodeId>& node_ids) {
    for(const auto& element_ref : element_refs) {
        const auto it = element_index.find(element_ref);
        if(it == element_index.end()) {
            continue;
        }
        for(uint8_t index = 0; index < it->second.nodeCount(); ++index) {
            const auto node_id = it->second.nodeId(index);
            if(node_id != INVALID_MESH_NODE_ID) {
                node_ids.emplace(node_id);
            }
        }
    }
}

void buildAdjacency(
    const std::unordered_map<MeshElementRef, MeshElement, MeshElementRefHash>& element_index,
    const MeshElementRefSet& element_refs,
    std::unordered_map<MeshNodeId, std::unordered_set<MeshNodeId>>& adjacency,
    std::unordered_map<uint64_t, size_t>& edge_use_counts) {
    for(const auto& element_ref : element_refs) {
        const auto it = element_index.find(element_ref);
        if(it == element_index.end()) {
            continue;
        }
        const auto [edges, count] = edgeTableForType(it->second.elementType());
        if(!edges) {
            continue;
        }

        for(size_t edge_index = 0; edge_index < count; ++edge_index) {
            const auto node_a = it->second.nodeId(edges[edge_index][0]);
            const auto node_b = it->second.nodeId(edges[edge_index][1]);
            if(node_a == INVALID_MESH_NODE_ID || node_b == INVALID_MESH_NODE_ID) {
                continue;
            }
            adjacency[node_a].insert(node_b);
            adjacency[node_b].insert(node_a);
            edge_use_counts[makeEdgeKey(node_a, node_b)] += 1;
        }
    }
}

[[nodiscard]] std::unordered_set<MeshNodeId>
collectBoundaryNodes(const std::unordered_map<uint64_t, size_t>& edge_use_counts) {
    std::unordered_set<MeshNodeId> boundary_nodes;
    for(const auto& [edge_key, use_count] : edge_use_counts) {
        if(use_count != 1) {
            continue;
        }
        const auto node_a = static_cast<MeshNodeId>(edge_key >> 32u);
        const auto node_b = static_cast<MeshNodeId>(edge_key & 0xFFFFFFFFu);
        boundary_nodes.emplace(node_a);
        boundary_nodes.emplace(node_b);
    }
    return boundary_nodes;
}

[[nodiscard]] Util::Pt3d
blendTowardsAverage(const Util::Pt3d& current,
                    const std::unordered_set<MeshNodeId>& neighbors,
                    const std::unordered_map<MeshNodeId, Util::Pt3d>& positions,
                    double factor) {
    if(neighbors.empty()) {
        return current;
    }

    Util::Vec3d accumulated{};
    size_t neighbor_count = 0;
    for(const auto neighbor_id : neighbors) {
        const auto position_it = positions.find(neighbor_id);
        if(position_it == positions.end()) {
            continue;
        }
        accumulated += position_it->second - Util::Pt3d{};
        ++neighbor_count;
    }

    if(neighbor_count == 0) {
        return current;
    }

    const auto average = Util::Pt3d(accumulated.x / static_cast<double>(neighbor_count),
                                    accumulated.y / static_cast<double>(neighbor_count),
                                    accumulated.z / static_cast<double>(neighbor_count));
    return current + (average - current) * factor;
}

double
applySmoothingPass(const std::unordered_set<MeshNodeId>& target_nodes,
                   const std::unordered_set<MeshNodeId>& locked_nodes,
                   const std::unordered_map<MeshNodeId, std::unordered_set<MeshNodeId>>& adjacency,
                   std::unordered_map<MeshNodeId, Util::Pt3d>& positions,
                   double factor) {
    std::unordered_map<MeshNodeId, Util::Pt3d> next_positions;
    next_positions.reserve(target_nodes.size());
    double max_displacement = 0.0;

    for(const auto node_id : target_nodes) {
        const auto current_it = positions.find(node_id);
        if(current_it == positions.end()) {
            continue;
        }
        if(locked_nodes.contains(node_id)) {
            continue;
        }
        const auto adjacency_it = adjacency.find(node_id);
        if(adjacency_it == adjacency.end() || adjacency_it->second.empty()) {
            continue;
        }

        const auto next_position =
            blendTowardsAverage(current_it->second, adjacency_it->second, positions, factor);
        max_displacement = std::max(max_displacement, current_it->second.distanceTo(next_position));
        next_positions.emplace(node_id, next_position);
    }

    for(const auto& [node_id, next_position] : next_positions) {
        positions[node_id] = next_position;
    }
    return max_displacement;
}

[[nodiscard]] nlohmann::json buildSummary(const SmoothSettings& settings,
                                          size_t moved_nodes,
                                          size_t target_nodes,
                                          size_t boundary_nodes,
                                          double max_displacement) {
    return {{"method", settings.m_method},
            {"iterations", settings.m_iterations},
            {"factor", settings.m_factor},
            {"preserveBoundaries", settings.m_preserveBoundaries},
            {"smoothedNodeCount", moved_nodes},
            {"targetNodeCount", target_nodes},
            {"boundaryNodeCount", boundary_nodes},
            {"maxDisplacement", max_displacement}};
}

} // namespace

nlohmann::json SmoothMeshAction::execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) {
    std::string error;
    const auto settings = parseSettings(params, error);
    if(!settings.has_value()) {
        return nlohmann::json{{"success", false}, {"error", error}};
    }

    const auto selection = parseTargets(params, error);
    if(!selection.has_value()) {
        return nlohmann::json{{"success", false}, {"error", error}};
    }

    auto document = MeshDocumentImpl::instance();
    if(!document) {
        return nlohmann::json{{"success", false}, {"error", "No active mesh document"}};
    }

    if(progress_callback && !progress_callback(0.1, "Collecting mesh region for smoothing...")) {
        return nlohmann::json{{"success", false}, {"error", "Operation canceled"}};
    }

    std::vector<MeshNode> nodes;
    std::vector<MeshElement> elements;
    document->snapshotMesh(nodes, elements);

    const auto node_index = buildNodeIndex(nodes);
    const auto element_index = buildElementIndex(elements);

    auto expanded_element_refs = selection->m_elementRefs;
    expandSelectedElements(*document, selection->m_nodeIds, selection->m_lineRefs,
                           expanded_element_refs);

    std::unordered_set<MeshNodeId> target_nodes = selection->m_nodeIds;
    collectLineNodes(elements, selection->m_lineRefs, target_nodes);
    collectTargetNodesFromElements(element_index, selection->m_elementRefs, target_nodes);
    for(const auto part_uid : selection->m_partUids) {
        collectPartElements(elements, part_uid, expanded_element_refs, target_nodes);
    }

    if(target_nodes.empty()) {
        collectTargetNodesFromElements(element_index, expanded_element_refs, target_nodes);
    }
    if(target_nodes.empty()) {
        return nlohmann::json{{"success", false},
                              {"error", "No mesh nodes resolved from smoothing targets"}};
    }

    std::unordered_map<MeshNodeId, std::unordered_set<MeshNodeId>> adjacency;
    std::unordered_map<uint64_t, size_t> edge_use_counts;
    buildAdjacency(element_index, expanded_element_refs, adjacency, edge_use_counts);

    std::unordered_map<MeshNodeId, Util::Pt3d> positions;
    positions.reserve(node_index.size());
    for(const auto& [node_id, node] : node_index) {
        positions.emplace(node_id, node.position());
    }

    const auto boundary_nodes = settings->m_preserveBoundaries
                                    ? collectBoundaryNodes(edge_use_counts)
                                    : std::unordered_set<MeshNodeId>{};

    if(progress_callback && !progress_callback(0.4, "Applying smoothing iterations...")) {
        return nlohmann::json{{"success", false}, {"error", "Operation canceled"}};
    }

    double max_displacement = 0.0;
    for(int iteration = 0; iteration < settings->m_iterations; ++iteration) {
        max_displacement =
            std::max(max_displacement, applySmoothingPass(target_nodes, boundary_nodes, adjacency,
                                                          positions, settings->m_factor));
        if(settings->m_method == "taubin") {
            max_displacement = std::max(max_displacement,
                                        applySmoothingPass(target_nodes, boundary_nodes, adjacency,
                                                           positions, -0.53 * settings->m_factor));
        }
    }

    std::unordered_map<MeshNodeId, Util::Pt3d> updated_positions;
    updated_positions.reserve(target_nodes.size());
    size_t moved_nodes = 0;
    for(const auto node_id : target_nodes) {
        const auto old_it = node_index.find(node_id);
        const auto new_it = positions.find(node_id);
        if(old_it == node_index.end() || new_it == positions.end()) {
            continue;
        }

        if(old_it->second.position().distanceTo(new_it->second) > 1e-9) {
            ++moved_nodes;
        }
        updated_positions.emplace(node_id, new_it->second);
    }

    if(updated_positions.empty()) {
        return nlohmann::json{{"success", false}, {"error", "No valid mesh nodes to smooth"}};
    }

    if(progress_callback && !progress_callback(0.8, "Updating mesh node positions...")) {
        return nlohmann::json{{"success", false}, {"error", "Operation canceled"}};
    }

    if(!document->updateNodePositions(updated_positions, error)) {
        return nlohmann::json{{"success", false}, {"error", error}};
    }

    auto response = buildSummary(settings.value(), moved_nodes, target_nodes.size(),
                                 boundary_nodes.size(), max_displacement);
    response["success"] = true;
    response["nodeCount"] = document->nodeCount();
    response["elementCount"] = document->elementCount();
    response["action"] = actionName();
    response["mesh_entities"] =
        nlohmann::json::array({{{"type", "Node"}, {"count", moved_nodes}},
                               {{"type", "Element"}, {"count", expanded_element_refs.size()}}});

    if(progress_callback) {
        progress_callback(1.0, "Mesh smoothing complete");
    }
    return response;
}

} // namespace OpenGeoLab::Mesh