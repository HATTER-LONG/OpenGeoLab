#include "mesh_render_builder.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/scene/pick_id.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace OpenGeoLab::Mesh {

namespace {

constexpr float K_DARKEN_FACTOR = 0.70F;
constexpr float K_NORMAL_OFFSET = 0.0005F;
constexpr std::array<float, 4> K_EDGE_COLOR = {0.10F, 0.10F, 0.10F, 1.0F};
constexpr std::array<float, 4> K_NODE_COLOR = {0.15F, 0.75F, 0.30F, 1.0F};

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

[[nodiscard]] std::array<float, 4> elementColorForShape(uint32_t shape_id) {
    const auto& base = Core::colorForShapeId(shape_id);
    return {base.r * K_DARKEN_FACTOR, base.g * K_DARKEN_FACTOR, base.b * K_DARKEN_FACTOR, base.a};
}

[[nodiscard]] Scene::RenderVertex makeVertex(const MeshNode& node,
                                             const std::array<float, 4>& color) {
    Scene::RenderVertex vertex;
    vertex.position[0] = node.position[0];
    vertex.position[1] = node.position[1];
    vertex.position[2] = node.position[2];
    vertex.color[0] = color[0];
    vertex.color[1] = color[1];
    vertex.color[2] = color[2];
    vertex.color[3] = color[3];
    return vertex;
}

[[nodiscard]] glm::vec3 vertexPosition(const MeshNode& node) {
    return {node.position[0], node.position[1], node.position[2]};
}

[[nodiscard]] glm::vec3 computeNormal(const MeshEntry& entry,
                                      const std::vector<uint32_t>& node_indices,
                                      const TriangleCorners& triangle) {
    const glm::vec3 p0 = vertexPosition(entry.nodes[node_indices[triangle[0]]]);
    const glm::vec3 p1 = vertexPosition(entry.nodes[node_indices[triangle[1]]]);
    const glm::vec3 p2 = vertexPosition(entry.nodes[node_indices[triangle[2]]]);
    const glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
    const float length = glm::length(normal);
    return length > 0.0F ? normal / length : glm::vec3{0.0F};
}

void assignNormal(Scene::RenderVertex& vertex, const glm::vec3& normal) {
    vertex.normal[0] = normal.x;
    vertex.normal[1] = normal.y;
    vertex.normal[2] = normal.z;
}

} // namespace

Scene::RenderMeshData MeshRenderBuilder::build(uint32_t shape_id, const MeshEntry& entry) {
    Scene::RenderMeshData result;
    result.version = entry.version;

    if(entry.nodes.empty()) {
        return result;
    }

    const auto element_color = elementColorForShape(shape_id);

    // Pre-pass: compute per-node averaged normals from adjacent elements.
    std::vector<glm::vec3> node_normals(entry.nodes.size(), glm::vec3{0.0F});
    {
        std::vector<uint32_t> tmp_indices;
        std::vector<TriangleCorners> tmp_triangles;
        tmp_triangles.reserve(12);
        for(const auto& element : entry.elements) {
            if(!collectElementNodeIndices(element, entry.nodes.size(), tmp_indices)) {
                continue;
            }
            tmp_triangles.clear();
            appendElementTriangles(element.type, tmp_triangles);
            for(const auto& triangle : tmp_triangles) {
                const glm::vec3 normal = computeNormal(entry, tmp_indices, triangle);
                for(const uint8_t corner : triangle) {
                    node_normals[tmp_indices[corner]] += normal;
                }
            }
        }
        for(auto& n : node_normals) {
            const float length = glm::length(n);
            if(length > 0.0F) {
                n /= length;
            }
        }
    }

    auto offsetPosition = [&](Scene::RenderVertex& vertex, std::size_t node_idx) {
        const glm::vec3& n = node_normals[node_idx];
        vertex.position[0] += n.x * K_NORMAL_OFFSET;
        vertex.position[1] += n.y * K_NORMAL_OFFSET;
        vertex.position[2] += n.z * K_NORMAL_OFFSET;
    };

    for(std::size_t node_index = 0; node_index < entry.nodes.size(); ++node_index) {
        const auto& node = entry.nodes[node_index];
        result.bounds.expand(glm::vec3{node.position[0], node.position[1], node.position[2]});
        auto vertex = makeVertex(node, K_NODE_COLOR);
        offsetPosition(vertex, node_index);
        result.vertices.push_back(vertex);
        result.pickIds.push_back({Scene::PickId::encode(shape_id, Core::EntityType::MeshNode,
                                                        static_cast<uint32_t>(node_index) + 1U)});
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

    std::vector<uint32_t> element_node_indices;
    std::vector<TriangleCorners> triangles;
    std::vector<EdgeCorners> element_edges;
    std::set<std::pair<uint32_t, uint32_t>> unique_edges;
    triangles.reserve(12);
    element_edges.reserve(12);

    for(std::size_t element_index = 0; element_index < entry.elements.size(); ++element_index) {
        const auto& element = entry.elements[element_index];
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

        triangles.clear();
        appendElementTriangles(element.type, triangles);
        if(triangles.empty()) {
            continue;
        }

        const uint32_t local_id = static_cast<uint32_t>(element_index) + 1U;
        const uint64_t pick_id =
            Scene::PickId::encode(shape_id, Core::EntityType::MeshElement, local_id);
        const uint32_t vertex_offset = static_cast<uint32_t>(result.vertices.size());
        const uint32_t index_offset = static_cast<uint32_t>(result.indices.size());
        for(const auto& triangle : triangles) {
            const glm::vec3 normal = computeNormal(entry, element_node_indices, triangle);
            for(const uint8_t corner : triangle) {
                auto vertex = makeVertex(entry.nodes[element_node_indices[corner]], element_color);
                // Offset along normal to prevent Z-fighting with geometry surface.
                vertex.position[0] += normal.x * K_NORMAL_OFFSET;
                vertex.position[1] += normal.y * K_NORMAL_OFFSET;
                vertex.position[2] += normal.z * K_NORMAL_OFFSET;
                assignNormal(vertex, normal);
                result.vertices.push_back(vertex);
                result.pickIds.push_back({pick_id});
                result.indices.push_back(static_cast<uint32_t>(result.vertices.size() - 1U));
            }
        }

        result.triangleRanges.push_back({
            .shapeId = shape_id,
            .entityType = Core::EntityType::MeshElement,
            .localId = local_id,
            .vertexOffset = vertex_offset,
            .vertexCount = static_cast<uint32_t>(triangles.size() * 3U),
            .indexOffset = index_offset,
            .indexCount = static_cast<uint32_t>(triangles.size() * 3U),
            .topology = Scene::PrimitiveTopology::Triangles,
        });
    }

    uint32_t edge_local_id = 1;
    for(const auto& [first, second] : unique_edges) {
        const uint64_t pick_id =
            Scene::PickId::encode(shape_id, Core::EntityType::MeshEdge, edge_local_id);
        const uint32_t vertex_offset = static_cast<uint32_t>(result.vertices.size());
        const uint32_t index_offset = static_cast<uint32_t>(result.indices.size());
        auto v0 = makeVertex(entry.nodes[first], K_EDGE_COLOR);
        offsetPosition(v0, first);
        result.vertices.push_back(v0);
        result.pickIds.push_back({pick_id});
        result.indices.push_back(static_cast<uint32_t>(result.vertices.size() - 1U));
        auto v1 = makeVertex(entry.nodes[second], K_EDGE_COLOR);
        offsetPosition(v1, second);
        result.vertices.push_back(v1);
        result.pickIds.push_back({pick_id});
        result.indices.push_back(static_cast<uint32_t>(result.vertices.size() - 1U));
        result.lineRanges.push_back({
            .shapeId = shape_id,
            .entityType = Core::EntityType::MeshEdge,
            .localId = edge_local_id++,
            .vertexOffset = vertex_offset,
            .vertexCount = 2U,
            .indexOffset = index_offset,
            .indexCount = 2U,
            .topology = Scene::PrimitiveTopology::Lines,
        });
    }

    assert(result.pickIds.size() == result.vertices.size());
    return result;
}

} // namespace OpenGeoLab::Mesh
