/**
 * @file mesh_document_impl.cpp
 * @brief Implementation of MeshDocumentImpl
 */

#include "mesh_document_impl.hpp"

#include "util/logger.hpp"

#include <algorithm>

namespace OpenGeoLab::Mesh {

MeshDocumentImpl::MeshDocumentImpl() { LOG_TRACE("MeshDocumentImpl created"); }

// =============================================================================
// Node Management
// =============================================================================

bool MeshDocumentImpl::addNode(const MeshNode& node) {
    std::lock_guard lock(m_mutex);

    if(m_nodeById.count(node.m_id) > 0 || m_nodeByUid.count(node.m_uid) > 0) {
        LOG_WARN("MeshDocumentImpl::addNode: duplicate node id={} uid={}", node.m_id, node.m_uid);
        return false;
    }

    const size_t idx = m_nodes.size();
    m_nodes.push_back(node);
    m_nodeById[node.m_id] = idx;
    m_nodeByUid[node.m_uid] = idx;

    // Update geometry mapping
    if(node.m_geometryEntity.isValid()) {
        m_geoToNodes[node.m_geometryEntity].push_back(node.key());
    }

    invalidateRenderCache();
    return true;
}

const MeshNode* MeshDocumentImpl::findNodeById(MeshElementId id) const {
    std::lock_guard lock(m_mutex);
    auto it = m_nodeById.find(id);
    return (it != m_nodeById.end()) ? &m_nodes[it->second] : nullptr;
}

const MeshNode* MeshDocumentImpl::findNodeByUID(MeshElementUID uid) const {
    std::lock_guard lock(m_mutex);
    auto it = m_nodeByUid.find(uid);
    return (it != m_nodeByUid.end()) ? &m_nodes[it->second] : nullptr;
}

size_t MeshDocumentImpl::nodeCount() const {
    std::lock_guard lock(m_mutex);
    return m_nodes.size();
}

std::vector<MeshNode> MeshDocumentImpl::allNodes() const {
    std::lock_guard lock(m_mutex);
    return m_nodes;
}

// =============================================================================
// Element Management
// =============================================================================

bool MeshDocumentImpl::addElement(const MeshElement& element) {
    std::lock_guard lock(m_mutex);

    if(m_elementById.count(element.m_id) > 0) {
        LOG_WARN("MeshDocumentImpl::addElement: duplicate element id={}", element.m_id);
        return false;
    }

    const TypeUidKey tuk{element.m_uid, element.m_type};
    if(m_elementByTypeUid.count(tuk) > 0) {
        LOG_WARN("MeshDocumentImpl::addElement: duplicate (uid={}, type={})", element.m_uid,
                 meshEntityTypeToString(element.m_type));
        return false;
    }

    const size_t idx = m_elements.size();
    m_elements.push_back(element);
    m_elementById[element.m_id] = idx;
    m_elementByTypeUid[tuk] = idx;

    // Update geometry mapping
    if(element.m_geometryEntity.isValid()) {
        m_geoToElements[element.m_geometryEntity].push_back(element.key());
    }

    invalidateRenderCache();
    return true;
}

const MeshElement* MeshDocumentImpl::findElementById(MeshElementId id) const {
    std::lock_guard lock(m_mutex);
    auto it = m_elementById.find(id);
    return (it != m_elementById.end()) ? &m_elements[it->second] : nullptr;
}

const MeshElement* MeshDocumentImpl::findElementByUID(MeshElementUID uid,
                                                      MeshEntityType type) const {
    std::lock_guard lock(m_mutex);
    auto it = m_elementByTypeUid.find({uid, type});
    return (it != m_elementByTypeUid.end()) ? &m_elements[it->second] : nullptr;
}

size_t MeshDocumentImpl::elementCount() const {
    std::lock_guard lock(m_mutex);
    return m_elements.size();
}

std::vector<MeshElement> MeshDocumentImpl::allElements() const {
    std::lock_guard lock(m_mutex);
    return m_elements;
}

std::vector<MeshElement> MeshDocumentImpl::elementsByType(MeshEntityType type) const {
    std::lock_guard lock(m_mutex);
    std::vector<MeshElement> result;
    for(const auto& elem : m_elements) {
        if(elem.m_type == type) {
            result.push_back(elem);
        }
    }
    return result;
}

// =============================================================================
// Geometry-Mesh Mapping
// =============================================================================

std::vector<MeshElementKey>
MeshDocumentImpl::findElementsByGeometry(const Geometry::EntityKey& geo_key) const {
    std::lock_guard lock(m_mutex);
    auto it = m_geoToElements.find(geo_key);
    return (it != m_geoToElements.end()) ? it->second : std::vector<MeshElementKey>{};
}

std::vector<MeshElementKey>
MeshDocumentImpl::findNodesByGeometry(const Geometry::EntityKey& geo_key) const {
    std::lock_guard lock(m_mutex);
    auto it = m_geoToNodes.find(geo_key);
    return (it != m_geoToNodes.end()) ? it->second : std::vector<MeshElementKey>{};
}

Geometry::EntityKey MeshDocumentImpl::geometryForElement(MeshElementUID elem_uid,
                                                         MeshEntityType elem_type) const {
    std::lock_guard lock(m_mutex);
    auto it = m_elementByTypeUid.find({elem_uid, elem_type});
    if(it != m_elementByTypeUid.end()) {
        return m_elements[it->second].m_geometryEntity;
    }
    return Geometry::EntityKey{};
}

Geometry::EntityKey MeshDocumentImpl::geometryForNode(MeshElementUID node_uid) const {
    std::lock_guard lock(m_mutex);
    auto it = m_nodeByUid.find(node_uid);
    if(it != m_nodeByUid.end()) {
        return m_nodes[it->second].m_geometryEntity;
    }
    return Geometry::EntityKey{};
}

// =============================================================================
// Render Data
// =============================================================================

Render::DocumentRenderData MeshDocumentImpl::getMeshRenderData() const {
    std::lock_guard lock(m_mutex);
    if(m_renderDataValid) {
        return m_renderDataCache;
    }
    m_renderDataCache = buildRenderData();
    m_renderDataValid = true;
    return m_renderDataCache;
}

Render::DocumentRenderData MeshDocumentImpl::buildRenderData() const {
    Render::DocumentRenderData data;

    if(m_nodes.empty()) {
        return data;
    }

    // Build uid → node index for fast lookup during element rendering
    std::unordered_map<MeshElementUID, size_t> uid_to_idx;
    for(size_t i = 0; i < m_nodes.size(); ++i) {
        uid_to_idx[m_nodes[i].m_uid] = i;
    }

    // Group elements by geometry face entity for per-face render meshes
    Geometry::EntityKeyMap<std::vector<size_t>> face_element_groups;
    std::vector<size_t> edge_element_indices;

    for(size_t i = 0; i < m_elements.size(); ++i) {
        const auto& elem = m_elements[i];
        if(elem.m_type == MeshEntityType::Triangle || elem.m_type == MeshEntityType::Quad) {
            face_element_groups[elem.m_geometryEntity].push_back(i);
        } else if(elem.m_type == MeshEntityType::Edge) {
            edge_element_indices.push_back(i);
        }
    }

    // ---- Generate face meshes (triangles/quads per geometry face) ----
    for(const auto& [geo_key, elem_indices] : face_element_groups) {
        Render::RenderMesh mesh;
        mesh.m_entityType = Geometry::EntityType::Face;
        mesh.m_entityUid = geo_key.m_uid;
        mesh.m_entityId = geo_key.m_id;
        mesh.m_primitiveType = Render::RenderPrimitiveType::Triangles;

        // Mesh element color: light blue-gray for mesh triangles
        const Render::RenderColor base_color{0.6f, 0.75f, 0.85f, 1.0f};
        mesh.m_baseColor = base_color;
        mesh.m_hoverColor = {0.9f, 0.95f, 1.0f, 1.0f};
        mesh.m_selectedColor = {0.3f, 0.6f, 1.0f, 1.0f};

        // Collect unique nodes and build local index
        std::unordered_map<MeshElementUID, uint32_t> local_node_map;

        for(size_t ei : elem_indices) {
            const auto& elem = m_elements[ei];
            for(MeshElementUID nuid : elem.m_nodeUids) {
                if(local_node_map.count(nuid) == 0) {
                    auto nit = uid_to_idx.find(nuid);
                    if(nit == uid_to_idx.end())
                        continue;
                    const auto& node = m_nodes[nit->second];
                    const auto local_idx = static_cast<uint32_t>(mesh.m_vertices.size());
                    local_node_map[nuid] = local_idx;

                    Render::RenderVertex rv;
                    rv.m_position[0] = static_cast<float>(node.m_position.m_x);
                    rv.m_position[1] = static_cast<float>(node.m_position.m_y);
                    rv.m_position[2] = static_cast<float>(node.m_position.m_z);
                    rv.m_color = base_color;
                    mesh.m_vertices.push_back(rv);
                    mesh.m_boundingBox.expand(node.m_position);
                }
            }
        }

        // Build index buffer and compute face normals
        for(size_t ei : elem_indices) {
            const auto& elem = m_elements[ei];
            if(elem.m_type == MeshEntityType::Triangle && elem.m_nodeUids.size() >= 3) {
                uint32_t i0 = local_node_map[elem.m_nodeUids[0]];
                uint32_t i1 = local_node_map[elem.m_nodeUids[1]];
                uint32_t i2 = local_node_map[elem.m_nodeUids[2]];
                mesh.m_indices.push_back(i0);
                mesh.m_indices.push_back(i1);
                mesh.m_indices.push_back(i2);

                // Compute face normal and accumulate on vertices
                auto& v0 = mesh.m_vertices[i0];
                auto& v1 = mesh.m_vertices[i1];
                auto& v2 = mesh.m_vertices[i2];
                float nx =
                    (v1.m_position[1] - v0.m_position[1]) * (v2.m_position[2] - v0.m_position[2]) -
                    (v1.m_position[2] - v0.m_position[2]) * (v2.m_position[1] - v0.m_position[1]);
                float ny =
                    (v1.m_position[2] - v0.m_position[2]) * (v2.m_position[0] - v0.m_position[0]) -
                    (v1.m_position[0] - v0.m_position[0]) * (v2.m_position[2] - v0.m_position[2]);
                float nz =
                    (v1.m_position[0] - v0.m_position[0]) * (v2.m_position[1] - v0.m_position[1]) -
                    (v1.m_position[1] - v0.m_position[1]) * (v2.m_position[0] - v0.m_position[0]);
                v0.m_normal[0] += nx;
                v0.m_normal[1] += ny;
                v0.m_normal[2] += nz;
                v1.m_normal[0] += nx;
                v1.m_normal[1] += ny;
                v1.m_normal[2] += nz;
                v2.m_normal[0] += nx;
                v2.m_normal[1] += ny;
                v2.m_normal[2] += nz;
            } else if(elem.m_type == MeshEntityType::Quad && elem.m_nodeUids.size() >= 4) {
                // Split quad into two triangles
                uint32_t i0 = local_node_map[elem.m_nodeUids[0]];
                uint32_t i1 = local_node_map[elem.m_nodeUids[1]];
                uint32_t i2 = local_node_map[elem.m_nodeUids[2]];
                uint32_t i3 = local_node_map[elem.m_nodeUids[3]];
                mesh.m_indices.push_back(i0);
                mesh.m_indices.push_back(i1);
                mesh.m_indices.push_back(i2);
                mesh.m_indices.push_back(i0);
                mesh.m_indices.push_back(i2);
                mesh.m_indices.push_back(i3);
            }
        }

        // Normalize accumulated normals
        for(auto& v : mesh.m_vertices) {
            float len = std::sqrt(v.m_normal[0] * v.m_normal[0] + v.m_normal[1] * v.m_normal[1] +
                                  v.m_normal[2] * v.m_normal[2]);
            if(len > 1e-6f) {
                v.m_normal[0] /= len;
                v.m_normal[1] /= len;
                v.m_normal[2] /= len;
            }
        }

        if(mesh.isValid()) {
            data.m_faceMeshes.push_back(std::move(mesh));
        }
    }

    // ---- Generate edge meshes (wireframe from element edges) ----
    // Extract unique edges from triangles/quads for wireframe display
    struct EdgePair {
        MeshElementUID a, b;
        bool operator==(const EdgePair& o) const {
            return (a == o.a && b == o.b) || (a == o.b && b == o.a);
        }
    };
    struct EdgePairHash {
        size_t operator()(const EdgePair& e) const {
            auto lo = std::min(e.a, e.b);
            auto hi = std::max(e.a, e.b);
            size_t h = std::hash<MeshElementUID>{}(lo);
            h ^= std::hash<MeshElementUID>{}(hi) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            return h;
        }
    };
    std::unordered_set<EdgePair, EdgePairHash> unique_edges;

    for(const auto& elem : m_elements) {
        if(elem.m_type == MeshEntityType::Triangle && elem.m_nodeUids.size() >= 3) {
            unique_edges.insert({elem.m_nodeUids[0], elem.m_nodeUids[1]});
            unique_edges.insert({elem.m_nodeUids[1], elem.m_nodeUids[2]});
            unique_edges.insert({elem.m_nodeUids[2], elem.m_nodeUids[0]});
        } else if(elem.m_type == MeshEntityType::Quad && elem.m_nodeUids.size() >= 4) {
            unique_edges.insert({elem.m_nodeUids[0], elem.m_nodeUids[1]});
            unique_edges.insert({elem.m_nodeUids[1], elem.m_nodeUids[2]});
            unique_edges.insert({elem.m_nodeUids[2], elem.m_nodeUids[3]});
            unique_edges.insert({elem.m_nodeUids[3], elem.m_nodeUids[0]});
        } else if(elem.m_type == MeshEntityType::Edge && elem.m_nodeUids.size() >= 2) {
            unique_edges.insert({elem.m_nodeUids[0], elem.m_nodeUids[1]});
        }
    }

    if(!unique_edges.empty()) {
        Render::RenderMesh edge_mesh;
        edge_mesh.m_entityType = Geometry::EntityType::Edge;
        edge_mesh.m_entityUid = Geometry::INVALID_ENTITY_UID;
        edge_mesh.m_primitiveType = Render::RenderPrimitiveType::Lines;
        edge_mesh.m_baseColor = {0.1f, 0.1f, 0.1f, 1.0f};
        edge_mesh.m_hoverColor = {1.0f, 0.8f, 0.0f, 1.0f};
        edge_mesh.m_selectedColor = {1.0f, 0.5f, 0.0f, 1.0f};

        for(const auto& ep : unique_edges) {
            auto it_a = uid_to_idx.find(ep.a);
            auto it_b = uid_to_idx.find(ep.b);
            if(it_a == uid_to_idx.end() || it_b == uid_to_idx.end())
                continue;

            const auto& na = m_nodes[it_a->second];
            const auto& nb = m_nodes[it_b->second];

            Render::RenderVertex va(static_cast<float>(na.m_position.m_x),
                                    static_cast<float>(na.m_position.m_y),
                                    static_cast<float>(na.m_position.m_z));
            va.setColor(0.1f, 0.1f, 0.1f);
            Render::RenderVertex vb(static_cast<float>(nb.m_position.m_x),
                                    static_cast<float>(nb.m_position.m_y),
                                    static_cast<float>(nb.m_position.m_z));
            vb.setColor(0.1f, 0.1f, 0.1f);

            auto base_idx = static_cast<uint32_t>(edge_mesh.m_vertices.size());
            edge_mesh.m_vertices.push_back(va);
            edge_mesh.m_vertices.push_back(vb);
            edge_mesh.m_indices.push_back(base_idx);
            edge_mesh.m_indices.push_back(base_idx + 1);

            edge_mesh.m_boundingBox.expand(na.m_position);
            edge_mesh.m_boundingBox.expand(nb.m_position);
        }

        if(edge_mesh.isValid()) {
            data.m_edgeMeshes.push_back(std::move(edge_mesh));
        }
    }

    // ---- Generate node meshes (points) ----
    if(!m_nodes.empty()) {
        Render::RenderMesh node_mesh;
        node_mesh.m_entityType = Geometry::EntityType::Vertex;
        node_mesh.m_entityUid = Geometry::INVALID_ENTITY_UID;
        node_mesh.m_primitiveType = Render::RenderPrimitiveType::Points;
        node_mesh.m_baseColor = {0.0f, 0.0f, 0.0f, 1.0f};
        node_mesh.m_hoverColor = {1.0f, 0.0f, 0.0f, 1.0f};
        node_mesh.m_selectedColor = {1.0f, 0.0f, 0.0f, 1.0f};

        for(const auto& node : m_nodes) {
            Render::RenderVertex rv(static_cast<float>(node.m_position.m_x),
                                    static_cast<float>(node.m_position.m_y),
                                    static_cast<float>(node.m_position.m_z));
            rv.setColor(0.0f, 0.0f, 0.0f);
            node_mesh.m_vertices.push_back(rv);
            node_mesh.m_boundingBox.expand(node.m_position);
        }

        if(node_mesh.isValid()) {
            data.m_vertexMeshes.push_back(std::move(node_mesh));
        }
    }

    data.updateBoundingBox();
    data.markModified();
    return data;
}

// =============================================================================
// Document Operations
// =============================================================================

void MeshDocumentImpl::clear() {
    std::lock_guard lock(m_mutex);
    m_nodes.clear();
    m_nodeById.clear();
    m_nodeByUid.clear();
    m_elements.clear();
    m_elementById.clear();
    m_elementByTypeUid.clear();
    m_geoToElements.clear();
    m_geoToNodes.clear();
    m_renderDataCache.clear();
    m_renderDataValid = false;
    LOG_TRACE("MeshDocumentImpl::clear: all mesh data cleared");
}

bool MeshDocumentImpl::isEmpty() const {
    std::lock_guard lock(m_mutex);
    return m_nodes.empty() && m_elements.empty();
}

void MeshDocumentImpl::invalidateRenderCache() { m_renderDataValid = false; }

} // namespace OpenGeoLab::Mesh
