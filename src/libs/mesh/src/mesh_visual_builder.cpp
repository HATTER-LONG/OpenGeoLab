/// @file mesh_visual_builder.cpp
/// @brief Converts MeshEntry data to Core::VisualData for GPU rendering.
///
/// 2D surface elements render directly as SurfaceMesh.
/// 3D volume elements extract boundary faces only (shared once = exterior).
/// Wireframe edges come from surface element edges + boundary face edges.
/// All nodes produce a PointSet.

#include <opengeolab/mesh/mesh_visual_builder.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace OpenGeoLab::Mesh::MeshVisualBuilder {

// ---------------------------------------------------------------------------
// Face definitions for volume element boundary extraction
// ---------------------------------------------------------------------------

/// Per-face definition: number of corners and their local indices.
struct FaceDef {
    uint8_t nNodes;
    uint8_t corners[4];
};

// Tetra4: 4 triangular faces
constexpr FaceDef K_TETRA_FACES[4] = {
    {3, {0, 2, 1, 0}},
    {3, {0, 1, 3, 0}},
    {3, {1, 2, 3, 0}},
    {3, {0, 3, 2, 0}},
};

// Hexa8: 6 quadrilateral faces (Gmsh node ordering: bottom 0123, top 4567)
constexpr FaceDef K_HEXA_FACES[6] = {
    {4, {0, 3, 2, 1}}, {4, {4, 5, 6, 7}}, {4, {0, 1, 5, 4}},
    {4, {2, 3, 7, 6}}, {4, {0, 4, 7, 3}}, {4, {1, 2, 6, 5}},
};

// Prism6: 2 triangular + 3 quadrilateral faces
constexpr FaceDef K_PRISM_FACES[5] = {
    {3, {0, 2, 1, 0}}, {3, {3, 4, 5, 0}}, {4, {0, 1, 4, 3}}, {4, {1, 2, 5, 4}}, {4, {0, 3, 5, 2}},
};

// Pyramid5: 1 quadrilateral + 4 triangular faces
constexpr FaceDef K_PYRAMID_FACES[5] = {
    {4, {0, 3, 2, 1}}, {3, {0, 1, 4, 0}}, {3, {1, 2, 4, 0}}, {3, {2, 3, 4, 0}}, {3, {0, 4, 3, 0}},
};

struct FaceTable {
    const FaceDef* faces;
    uint8_t count;
};

static FaceTable faceTableFor(ElementType type) {
    switch(type) {
    case ElementType::Tetra4:
    case ElementType::Tetra10:
        return {K_TETRA_FACES, 4};
    case ElementType::Hexa8:
    case ElementType::Hexa27:
        return {K_HEXA_FACES, 6};
    case ElementType::Prism6:
    case ElementType::Prism18:
        return {K_PRISM_FACES, 5};
    case ElementType::Pyramid5:
    case ElementType::Pyramid14:
        return {K_PYRAMID_FACES, 5};
    default:
        return {nullptr, 0};
    }
}

// ---------------------------------------------------------------------------
// Face / edge key types for deduplication
// ---------------------------------------------------------------------------

/// Oriented face: preserves winding order for normal computation.
struct OrientedFace {
    uint32_t nodeIds[4]; ///< 1-based node IDs in winding order
    uint8_t count;       ///< 3 for triangle, 4 for quad
};

/// Canonical face key: node IDs sorted for comparison.
struct FaceKey {
    uint32_t n[4]{};
    uint8_t count{};

    bool operator==(const FaceKey& other) const {
        if(count != other.count) {
            return false;
        }
        for(uint8_t i = 0; i < count; ++i) {
            if(n[i] != other.n[i]) {
                return false;
            }
        }
        return true;
    }
};

struct FaceKeyHash {
    size_t operator()(const FaceKey& k) const {
        size_t h = 14695981039346656037ULL;
        for(uint8_t i = 0; i < k.count; ++i) {
            h ^= static_cast<size_t>(k.n[i]);
            h *= 1099511628211ULL;
        }
        return h;
    }
};

static FaceKey makeCanonicalKey(const uint32_t* node_ids, uint8_t count) {
    FaceKey key{};
    key.count = count;
    for(uint8_t i = 0; i < count; ++i) {
        key.n[i] = node_ids[i];
    }
    std::sort(key.n, key.n + count);
    return key;
}

struct EdgeKey {
    uint32_t a, b; ///< sorted (a < b)

    bool operator==(const EdgeKey& other) const { return a == other.a && b == other.b; }
};

struct EdgeKeyHash {
    size_t operator()(const EdgeKey& k) const {
        return std::hash<uint64_t>{}(static_cast<uint64_t>(k.a) << 32 | k.b);
    }
};

static EdgeKey makeEdgeKey(uint32_t n0, uint32_t n1) {
    return n0 < n1 ? EdgeKey{n0, n1} : EdgeKey{n1, n0};
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

static std::array<float, 3>
triangleNormal(const MeshNodeArray& nodes, uint32_t n0, uint32_t n1, uint32_t n2) {
    auto p0 = nodes.position(n0);
    auto p1 = nodes.position(n1);
    auto p2 = nodes.position(n2);

    float const ux = static_cast<float>(p1[0] - p0[0]);
    float const uy = static_cast<float>(p1[1] - p0[1]);
    float const uz = static_cast<float>(p1[2] - p0[2]);
    float const vx = static_cast<float>(p2[0] - p0[0]);
    float const vy = static_cast<float>(p2[1] - p0[1]);
    float const vz = static_cast<float>(p2[2] - p0[2]);

    float nx = uy * vz - uz * vy;
    float ny = uz * vx - ux * vz;
    float nz = ux * vy - uy * vx;

    float const len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if(len > 1e-10f) {
        nx /= len;
        ny /= len;
        nz /= len;
    }
    return {nx, ny, nz};
}

static void pushPosition(std::vector<float>& out, const MeshNodeArray& nodes, uint32_t node_id) {
    auto pos = nodes.position(node_id);
    out.push_back(static_cast<float>(pos[0]));
    out.push_back(static_cast<float>(pos[1]));
    out.push_back(static_cast<float>(pos[2]));
}

static void pushNormal(std::vector<float>& out, const std::array<float, 3>& n) {
    out.push_back(n[0]);
    out.push_back(n[1]);
    out.push_back(n[2]);
}

// ---------------------------------------------------------------------------
// 2D surface elements → SurfaceMesh (direct rendering)
// ---------------------------------------------------------------------------

static Core::SurfaceMesh buildSurfaceFromBlocks(const std::vector<ElementBlock>& surface_blocks,
                                                const MeshNodeArray& nodes) {
    Core::SurfaceMesh mesh;
    uint32_t vertex_offset = 0;

    for(const auto& block : surface_blocks) {
        if(elementDimension(block.type) != 2) {
            continue;
        }

        const auto npe = block.nodesPerElem();
        const auto elem_count = block.elementCount();
        // Corner count: 3 for tris, 4 for quads (higher-order uses only corners)
        const bool is_tri =
            (block.type == ElementType::Triangle3 || block.type == ElementType::Triangle6);

        for(size_t e = 0; e < elem_count; ++e) {
            const auto* conn = block.connectivity.data() + e * npe;

            if(is_tri) {
                auto normal = triangleNormal(nodes, conn[0], conn[1], conn[2]);
                for(uint32_t v = 0; v < 3; ++v) {
                    pushPosition(mesh.positions, nodes, conn[v]);
                    pushNormal(mesh.normals, normal);
                }
                mesh.indices.push_back(vertex_offset);
                mesh.indices.push_back(vertex_offset + 1);
                mesh.indices.push_back(vertex_offset + 2);
                vertex_offset += 3;
            } else {
                // Quad → 2 triangles: (0,1,2) and (0,2,3)
                auto normal = triangleNormal(nodes, conn[0], conn[1], conn[2]);
                for(uint32_t v = 0; v < 4; ++v) {
                    pushPosition(mesh.positions, nodes, conn[v]);
                    pushNormal(mesh.normals, normal);
                }
                mesh.indices.push_back(vertex_offset);
                mesh.indices.push_back(vertex_offset + 1);
                mesh.indices.push_back(vertex_offset + 2);
                mesh.indices.push_back(vertex_offset);
                mesh.indices.push_back(vertex_offset + 2);
                mesh.indices.push_back(vertex_offset + 3);
                vertex_offset += 4;
            }
        }
    }

    mesh.defaultColor[0] = 0.5f;
    mesh.defaultColor[1] = 0.7f;
    mesh.defaultColor[2] = 0.9f;
    mesh.defaultColor[3] = 1.0f;
    return mesh;
}

// ---------------------------------------------------------------------------
// 3D volume boundary extraction
// ---------------------------------------------------------------------------

/// Extract boundary faces (shared once) from volume element blocks.
static std::vector<OrientedFace>
extractBoundaryFaces(const std::vector<ElementBlock>& volume_blocks) {
    struct FaceRecord {
        OrientedFace face;
        uint32_t count = 0;
    };

    std::unordered_map<FaceKey, FaceRecord, FaceKeyHash> face_map;

    for(const auto& block : volume_blocks) {
        auto table = faceTableFor(block.type);
        if(!table.faces) {
            continue;
        }

        const auto npe = block.nodesPerElem();
        const auto elem_count = block.elementCount();

        for(size_t e = 0; e < elem_count; ++e) {
            const auto* conn = block.connectivity.data() + e * npe;

            for(uint8_t f = 0; f < table.count; ++f) {
                const auto& fd = table.faces[f];
                uint32_t face_nodes[4];
                for(uint8_t i = 0; i < fd.nNodes; ++i) {
                    face_nodes[i] = conn[fd.corners[i]];
                }

                auto key = makeCanonicalKey(face_nodes, fd.nNodes);
                auto& record = face_map[key];
                if(record.count == 0) {
                    for(uint8_t i = 0; i < fd.nNodes; ++i) {
                        record.face.nodeIds[i] = face_nodes[i];
                    }
                    record.face.count = fd.nNodes;
                }
                ++record.count;
            }
        }
    }

    std::vector<OrientedFace> boundary;
    boundary.reserve(face_map.size() / 4); // rough estimate
    for(const auto& [key, record] : face_map) {
        if(record.count == 1) {
            boundary.push_back(record.face);
        }
    }
    return boundary;
}

/// Build SurfaceMesh from extracted boundary faces.
static Core::SurfaceMesh buildSurfaceFromFaces(const std::vector<OrientedFace>& faces,
                                               const MeshNodeArray& nodes) {
    Core::SurfaceMesh mesh;
    uint32_t vertex_offset = 0;

    for(const auto& face : faces) {
        auto normal = triangleNormal(nodes, face.nodeIds[0], face.nodeIds[1], face.nodeIds[2]);

        if(face.count == 3) {
            for(uint8_t v = 0; v < 3; ++v) {
                pushPosition(mesh.positions, nodes, face.nodeIds[v]);
                pushNormal(mesh.normals, normal);
            }
            mesh.indices.push_back(vertex_offset);
            mesh.indices.push_back(vertex_offset + 1);
            mesh.indices.push_back(vertex_offset + 2);
            vertex_offset += 3;
        } else {
            for(uint8_t v = 0; v < 4; ++v) {
                pushPosition(mesh.positions, nodes, face.nodeIds[v]);
                pushNormal(mesh.normals, normal);
            }
            mesh.indices.push_back(vertex_offset);
            mesh.indices.push_back(vertex_offset + 1);
            mesh.indices.push_back(vertex_offset + 2);
            mesh.indices.push_back(vertex_offset);
            mesh.indices.push_back(vertex_offset + 2);
            mesh.indices.push_back(vertex_offset + 3);
            vertex_offset += 4;
        }
    }

    mesh.defaultColor[0] = 0.5f;
    mesh.defaultColor[1] = 0.7f;
    mesh.defaultColor[2] = 0.9f;
    mesh.defaultColor[3] = 1.0f;
    return mesh;
}

// ---------------------------------------------------------------------------
// Wireframe edge extraction
// ---------------------------------------------------------------------------

/// Collect unique edges from 2D surface element blocks (all element edges visible).
static void collectSurfaceEdges(const std::vector<ElementBlock>& surface_blocks,
                                std::unordered_set<EdgeKey, EdgeKeyHash>& edges) {
    for(const auto& block : surface_blocks) {
        if(elementDimension(block.type) != 2) {
            continue;
        }

        const auto npe = block.nodesPerElem();
        const auto elem_count = block.elementCount();
        const bool is_tri =
            (block.type == ElementType::Triangle3 || block.type == ElementType::Triangle6);
        const uint32_t corners = is_tri ? 3u : 4u;

        for(size_t e = 0; e < elem_count; ++e) {
            const auto* conn = block.connectivity.data() + e * npe;
            for(uint32_t i = 0; i < corners; ++i) {
                edges.insert(makeEdgeKey(conn[i], conn[(i + 1) % corners]));
            }
        }
    }
}

/// Collect unique edges from boundary faces only.
static void collectBoundaryEdges(const std::vector<OrientedFace>& boundary_faces,
                                 std::unordered_set<EdgeKey, EdgeKeyHash>& edges) {
    for(const auto& face : boundary_faces) {
        for(uint8_t i = 0; i < face.count; ++i) {
            edges.insert(makeEdgeKey(face.nodeIds[i], face.nodeIds[(i + 1) % face.count]));
        }
    }
}

/// Build EdgeMesh from unique edges.
static Core::EdgeMesh buildEdgeMesh(const std::unordered_set<EdgeKey, EdgeKeyHash>& unique_edges,
                                    const MeshNodeArray& nodes) {
    Core::EdgeMesh mesh;
    std::unordered_map<uint32_t, uint32_t> node_to_vertex;

    auto get_vertex = [&](uint32_t node_id) -> uint32_t {
        auto it = node_to_vertex.find(node_id);
        if(it != node_to_vertex.end()) {
            return it->second;
        }
        auto idx = static_cast<uint32_t>(node_to_vertex.size());
        node_to_vertex[node_id] = idx;
        pushPosition(mesh.positions, nodes, node_id);
        return idx;
    };

    for(const auto& edge : unique_edges) {
        mesh.indices.push_back(get_vertex(edge.a));
        mesh.indices.push_back(get_vertex(edge.b));
    }

    mesh.color[0] = 0.2f;
    mesh.color[1] = 0.2f;
    mesh.color[2] = 0.2f;
    mesh.color[3] = 1.0f;
    return mesh;
}

// ---------------------------------------------------------------------------
// Node point cloud
// ---------------------------------------------------------------------------

static Core::PointSet buildPointCloud(const MeshNodeArray& nodes) {
    Core::PointSet ps;
    ps.positions.reserve(nodes.count() * 3);
    for(size_t i = 0; i < nodes.count(); ++i) {
        auto pos = nodes.position(static_cast<uint32_t>(i + 1));
        ps.positions.push_back(static_cast<float>(pos[0]));
        ps.positions.push_back(static_cast<float>(pos[1]));
        ps.positions.push_back(static_cast<float>(pos[2]));
    }
    ps.pointSize = 3.0f;
    ps.color[0] = 0.0f;
    ps.color[1] = 0.0f;
    ps.color[2] = 0.0f;
    ps.color[3] = 1.0f;
    return ps;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

Core::VisualData buildVisualData(const MeshEntry& entry) {
    Core::VisualData visual;

    // Surface mesh from 2D elements (direct rendering)
    if(!entry.surfaceBlocks.empty()) {
        visual.surfaces.push_back(buildSurfaceFromBlocks(entry.surfaceBlocks, entry.nodes));
    }

    // Surface mesh from 3D volume boundary faces
    std::vector<OrientedFace> boundary_faces;
    if(!entry.volumeBlocks.empty()) {
        boundary_faces = extractBoundaryFaces(entry.volumeBlocks);
        visual.surfaces.push_back(buildSurfaceFromFaces(boundary_faces, entry.nodes));
    }

    // Wireframe edges: 2D element edges + 3D boundary face edges
    std::unordered_set<EdgeKey, EdgeKeyHash> unique_edges;
    collectSurfaceEdges(entry.surfaceBlocks, unique_edges);
    collectBoundaryEdges(boundary_faces, unique_edges);
    if(!unique_edges.empty()) {
        visual.edges.push_back(buildEdgeMesh(unique_edges, entry.nodes));
    }

    // Node point cloud
    if(entry.nodes.count() > 0) {
        visual.points.push_back(buildPointCloud(entry.nodes));
    }

    visual.style = Core::RenderStyle::SolidWithEdges;
    return visual;
}

MeshTags buildEntityTags(const MeshEntry& entry) {
    MeshTags tags;

    // Node tags: one per node, 1-based ID
    tags.nodeTags.reserve(entry.nodes.count());
    for(size_t i = 0; i < entry.nodes.count(); ++i) {
        tags.nodeTags.push_back({Core::EntityType::MeshNode, static_cast<uint32_t>(i + 1)});
    }

    // Element tags follow the ElementLocator global ID order: line → surface → volume
    uint32_t global_elem_id = 1;

    for(const auto& block : entry.lineBlocks) {
        for(size_t e = 0; e < block.elementCount(); ++e) {
            tags.edgeTags.push_back({Core::EntityType::MeshEdge, global_elem_id++});
        }
    }

    for(const auto& block : entry.surfaceBlocks) {
        for(size_t e = 0; e < block.elementCount(); ++e) {
            tags.elementTags.push_back({Core::EntityType::MeshElement, global_elem_id++});
        }
    }

    for(const auto& block : entry.volumeBlocks) {
        for(size_t e = 0; e < block.elementCount(); ++e) {
            tags.elementTags.push_back({Core::EntityType::MeshElement, global_elem_id++});
        }
    }

    return tags;
}

} // namespace OpenGeoLab::Mesh::MeshVisualBuilder
