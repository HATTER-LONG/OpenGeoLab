#include "mesh_render_builder.hpp"

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/pick_id.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>
#include <vector>

namespace OpenGeoLab::Mesh {

namespace {

constexpr std::array<float, 4> K_MESH_COLOR = {0.75F, 0.78F, 0.82F, 1.0F};

using TriangleCorners = std::array<uint8_t, 3>;
using EdgeCorners = std::array<uint8_t, 2>;

[[nodiscard]] bool isValidLocalId(uint32_t local_id, std::size_t node_count) {
    return local_id > 0U && local_id <= node_count;
}

[[nodiscard]] bool collectElementNodeIndices(const MeshElement& element,
                                             std::size_t node_count,
                                             std::vector<uint32_t>& node_indices) {
    node_indices.clear();
    node_indices.reserve(nodeCount(element.type));

    for(uint8_t i = 0; i < nodeCount(element.type); ++i) {
        const uint32_t local_id = element.nodeLocalIds[i];
        if(!isValidLocalId(local_id, node_count)) {
            return false;
        }
        node_indices.push_back(local_id - 1U);
    }

    return true;
}

void appendElementTriangles(MeshElementType type, std::vector<TriangleCorners>& triangles) {
    switch(type) {
    case MeshElementType::Triangle:
        triangles.push_back({0, 1, 2});
        return;
    case MeshElementType::Quad:
        triangles.push_back({0, 1, 2});
        triangles.push_back({0, 2, 3});
        return;
    case MeshElementType::Tetra:
        triangles.push_back({0, 1, 2});
        triangles.push_back({0, 3, 1});
        triangles.push_back({1, 3, 2});
        triangles.push_back({0, 2, 3});
        return;
    case MeshElementType::Hexa:
        triangles.push_back({0, 1, 2});
        triangles.push_back({0, 2, 3});
        triangles.push_back({4, 5, 6});
        triangles.push_back({4, 6, 7});
        triangles.push_back({0, 4, 5});
        triangles.push_back({0, 5, 1});
        triangles.push_back({1, 5, 6});
        triangles.push_back({1, 6, 2});
        triangles.push_back({2, 6, 7});
        triangles.push_back({2, 7, 3});
        triangles.push_back({3, 7, 4});
        triangles.push_back({3, 4, 0});
        return;
    case MeshElementType::Prism:
        triangles.push_back({0, 1, 2});
        triangles.push_back({3, 5, 4});
        triangles.push_back({0, 1, 4});
        triangles.push_back({0, 4, 3});
        triangles.push_back({1, 2, 5});
        triangles.push_back({1, 5, 4});
        triangles.push_back({2, 0, 3});
        triangles.push_back({2, 3, 5});
        return;
    case MeshElementType::Pyramid:
        triangles.push_back({0, 1, 2});
        triangles.push_back({0, 2, 3});
        triangles.push_back({0, 1, 4});
        triangles.push_back({1, 2, 4});
        triangles.push_back({2, 3, 4});
        triangles.push_back({3, 0, 4});
        return;
    }
}

void appendElementEdges(MeshElementType type, std::vector<EdgeCorners>& edges) {
    switch(type) {
    case MeshElementType::Triangle:
        edges.insert(edges.end(), {{0, 1}, {1, 2}, {2, 0}});
        return;
    case MeshElementType::Quad:
        edges.insert(edges.end(), {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
        return;
    case MeshElementType::Tetra:
        edges.insert(edges.end(), {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}});
        return;
    case MeshElementType::Hexa:
        edges.insert(edges.end(), {{0, 1},
                                   {1, 2},
                                   {2, 3},
                                   {3, 0},
                                   {4, 5},
                                   {5, 6},
                                   {6, 7},
                                   {7, 4},
                                   {0, 4},
                                   {1, 5},
                                   {2, 6},
                                   {3, 7}});
        return;
    case MeshElementType::Prism:
        edges.insert(edges.end(),
                     {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}, {0, 3}, {1, 4}, {2, 5}});
        return;
    case MeshElementType::Pyramid:
        edges.insert(edges.end(), {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}});
        return;
    }
}

[[nodiscard]] glm::vec3 vertexPosition(const Scene::RenderVertex& vertex) {
    return {vertex.position[0], vertex.position[1], vertex.position[2]};
}

[[nodiscard]] glm::vec3 computeNormal(const Scene::RenderMeshData& data,
                                      const std::vector<uint32_t>& node_indices,
                                      const TriangleCorners& triangle) {
    const glm::vec3 p0 = vertexPosition(data.vertices[node_indices[triangle[0]]]);
    const glm::vec3 p1 = vertexPosition(data.vertices[node_indices[triangle[1]]]);
    const glm::vec3 p2 = vertexPosition(data.vertices[node_indices[triangle[2]]]);
    const glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
    const float length = glm::length(normal);
    return length > 0.0F ? normal / length : glm::vec3{0.0F};
}

void assignNormal(Scene::RenderVertex& vertex, const glm::vec3& normal) {
    vertex.normal[0] = normal.x;
    vertex.normal[1] = normal.y;
    vertex.normal[2] = normal.z;
}

[[nodiscard]] uint32_t minNodeIndex(const std::vector<uint32_t>& node_indices) {
    return *std::min_element(node_indices.begin(), node_indices.end());
}

[[nodiscard]] uint32_t maxNodeIndex(const std::vector<uint32_t>& node_indices) {
    return *std::max_element(node_indices.begin(), node_indices.end());
}

} // namespace

Scene::RenderMeshData MeshRenderBuilder::build(uint32_t shape_id, const MeshEntry& entry) {
    Scene::RenderMeshData result;
    result.version = entry.version;

    if(entry.nodes.empty()) {
        return result;
    }

    result.vertices.reserve(entry.nodes.size());
    result.pickIds.resize(entry.nodes.size());

    for(const auto& node : entry.nodes) {
        Scene::RenderVertex vertex;
        vertex.position[0] = node.position[0];
        vertex.position[1] = node.position[1];
        vertex.position[2] = node.position[2];
        vertex.color[0] = K_MESH_COLOR[0];
        vertex.color[1] = K_MESH_COLOR[1];
        vertex.color[2] = K_MESH_COLOR[2];
        vertex.color[3] = K_MESH_COLOR[3];
        result.bounds.expand(glm::vec3{node.position[0], node.position[1], node.position[2]});
        result.vertices.push_back(vertex);
    }

    std::vector<uint32_t> element_node_indices;
    std::vector<TriangleCorners> triangles;
    triangles.reserve(12);

    for(std::size_t element_index = 0; element_index < entry.elements.size(); ++element_index) {
        const auto& element = entry.elements[element_index];
        if(!collectElementNodeIndices(element, entry.nodes.size(), element_node_indices)) {
            continue;
        }

        triangles.clear();
        appendElementTriangles(element.type, triangles);
        if(triangles.empty()) {
            continue;
        }

        const uint32_t local_id = static_cast<uint32_t>(element_index) + 1U;
        const uint64_t pick_id =
            Scene::PickId::encode(shape_id, Core::EntityType::MeshElement, local_id);
        const glm::vec3 normal = computeNormal(result, element_node_indices, triangles.front());

        for(const uint32_t node_index : element_node_indices) {
            assignNormal(result.vertices[node_index], normal);
            result.pickIds[node_index].pickId = pick_id;
        }

        const uint32_t index_offset = static_cast<uint32_t>(result.indices.size());
        for(const auto& triangle : triangles) {
            result.indices.push_back(element_node_indices[triangle[0]]);
            result.indices.push_back(element_node_indices[triangle[1]]);
            result.indices.push_back(element_node_indices[triangle[2]]);
        }

        const uint32_t min_index = minNodeIndex(element_node_indices);
        const uint32_t max_index = maxNodeIndex(element_node_indices);
        result.triangleRanges.push_back({
            .shapeId = shape_id,
            .entityType = Core::EntityType::MeshElement,
            .localId = local_id,
            .vertexOffset = min_index,
            .vertexCount = max_index - min_index + 1U,
            .indexOffset = index_offset,
            .indexCount = static_cast<uint32_t>(triangles.size() * 3U),
            .topology = Scene::PrimitiveTopology::Triangles,
        });
    }

    std::set<std::pair<uint32_t, uint32_t>> unique_edges;
    std::vector<EdgeCorners> element_edges;
    element_edges.reserve(12);

    for(const auto& element : entry.elements) {
        if(!collectElementNodeIndices(element, entry.nodes.size(), element_node_indices)) {
            continue;
        }

        element_edges.clear();
        appendElementEdges(element.type, element_edges);
        for(const auto& edge : element_edges) {
            const uint32_t first = element_node_indices[edge[0]];
            const uint32_t second = element_node_indices[edge[1]];
            unique_edges.emplace(std::min(first, second), std::max(first, second));
        }
    }

    uint32_t edge_local_id = 1;
    for(const auto& [first, second] : unique_edges) {
        const uint32_t index_offset = static_cast<uint32_t>(result.indices.size());
        result.indices.push_back(first);
        result.indices.push_back(second);
        result.lineRanges.push_back({
            .shapeId = shape_id,
            .entityType = Core::EntityType::MeshEdge,
            .localId = edge_local_id++,
            .vertexOffset = first,
            .vertexCount = second - first + 1U,
            .indexOffset = index_offset,
            .indexCount = 2U,
            .topology = Scene::PrimitiveTopology::Lines,
        });
    }

    for(std::size_t node_index = 0; node_index < entry.nodes.size(); ++node_index) {
        result.pointRanges.push_back({
            .shapeId = shape_id,
            .entityType = Core::EntityType::MeshNode,
            .localId = static_cast<uint32_t>(node_index) + 1U,
            .vertexOffset = static_cast<uint32_t>(node_index),
            .vertexCount = 1U,
            .indexOffset = 0U,
            .indexCount = 0U,
            .topology = Scene::PrimitiveTopology::Points,
        });
    }

    return result;
}

} // namespace OpenGeoLab::Mesh
