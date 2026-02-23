/**
 * @file mesh_documentImpl.cpp
 * @brief Concrete implementation of MeshDocument
 */

#include "mesh_documentImpl.hpp"

#include "render/render_types.hpp"
#include "util/logger.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Mesh {

// =============================================================================
// Singleton
// =============================================================================

std::shared_ptr<MeshDocumentImpl> MeshDocumentImpl::instance() {
    static auto s_instance = std::make_shared<MeshDocumentImpl>();
    return s_instance;
}

// =============================================================================
// Node Management
// =============================================================================

bool MeshDocumentImpl::addNode(const MeshNode& node) {
    const MeshNodeId id = node.nodeId();
    if(id == INVALID_MESH_NODE_ID) {
        return false;
    }
    auto [it, inserted] = m_nodes.emplace(id, node);
    return inserted;
}

const MeshNode* MeshDocumentImpl::findNodeById(MeshNodeId node_id) const {
    const auto it = m_nodes.find(node_id);
    return (it != m_nodes.end()) ? &it->second : nullptr;
}

size_t MeshDocumentImpl::nodeCount() const { return m_nodes.size(); }

// =============================================================================
// Element Management
// =============================================================================

bool MeshDocumentImpl::addElement(MeshElement element) {
    const MeshElementId id = element.elementId();
    if(id == INVALID_MESH_ELEMENT_ID) {
        return false;
    }
    if(m_elements.count(id) > 0) {
        return false;
    }
    const MeshElementRef ref = element.elementRef();
    const MeshElementUID uid = element.elementUID();
    m_refToId.emplace(ref, id);
    m_uidToId.emplace(uid, id);

    // Build reverse index: node -> elements
    const uint8_t nc = element.nodeCount();
    for(uint8_t i = 0; i < nc; ++i) {
        const MeshNodeId nid = element.nodeId(i);
        if(nid != INVALID_MESH_NODE_ID) {
            m_nodeToElements[nid].insert(id);
        }
    }

    m_elements.emplace(id, std::move(element));
    return true;
}

const MeshElement* MeshDocumentImpl::findElementById(MeshElementId element_id) const {
    const auto it = m_elements.find(element_id);
    return (it != m_elements.end()) ? &it->second : nullptr;
}

const MeshElement* MeshDocumentImpl::findElementByRef(const MeshElementRef& ref) const {
    const auto it = m_refToId.find(ref);
    if(it == m_refToId.end()) {
        return nullptr;
    }
    return findElementById(it->second);
}

size_t MeshDocumentImpl::elementCount() const { return m_elements.size(); }

std::vector<const MeshElement*> MeshDocumentImpl::elementsByType(MeshElementType type) const {
    std::vector<const MeshElement*> result;
    for(const auto& [id, elem] : m_elements) {
        if(elem.elementType() == type) {
            result.push_back(&elem);
        }
    }
    return result;
}

// =============================================================================
// Clear
// =============================================================================

void MeshDocumentImpl::clear() {
    m_nodes.clear();
    m_elements.clear();
    m_refToId.clear();
    m_nodeToElements.clear();
    m_uidToId.clear();
    LOG_DEBUG("MeshDocumentImpl: Cleared all nodes and elements");
}

// =============================================================================
// Topology Queries
// =============================================================================

std::vector<MeshElementId> MeshDocumentImpl::findElementsByNodeId(MeshNodeId node_id) const {
    const auto it = m_nodeToElements.find(node_id);
    if(it == m_nodeToElements.end()) {
        return {};
    }
    return {it->second.begin(), it->second.end()};
}

std::vector<MeshNodeId> MeshDocumentImpl::findAdjacentNodes(MeshNodeId node_id) const {
    const auto it = m_nodeToElements.find(node_id);
    if(it == m_nodeToElements.end()) {
        return {};
    }
    std::unordered_set<MeshNodeId> result;
    for(const MeshElementId eid : it->second) {
        const auto* elem = findElementById(eid);
        if(!elem) {
            continue;
        }
        const uint8_t nc = elem->nodeCount();
        for(uint8_t i = 0; i < nc; ++i) {
            const MeshNodeId nid = elem->nodeId(i);
            if(nid != node_id && nid != INVALID_MESH_NODE_ID) {
                result.insert(nid);
            }
        }
    }
    return {result.begin(), result.end()};
}

std::vector<MeshElementId> MeshDocumentImpl::findAdjacentElements(MeshElementId element_id) const {
    const auto* elem = findElementById(element_id);
    if(!elem) {
        return {};
    }
    std::unordered_set<MeshElementId> result;
    const uint8_t nc = elem->nodeCount();
    for(uint8_t i = 0; i < nc; ++i) {
        const MeshNodeId nid = elem->nodeId(i);
        const auto it = m_nodeToElements.find(nid);
        if(it != m_nodeToElements.end()) {
            for(const MeshElementId adj_eid : it->second) {
                if(adj_eid != element_id) {
                    result.insert(adj_eid);
                }
            }
        }
    }
    return {result.begin(), result.end()};
}

const MeshElement* MeshDocumentImpl::findElementByUID(MeshElementUID uid) const {
    const auto it = m_uidToId.find(uid);
    if(it == m_uidToId.end()) {
        return nullptr;
    }
    return findElementById(it->second);
}

// =============================================================================
// Render Data
// =============================================================================

Render::DocumentRenderData MeshDocumentImpl::getRenderData() const {
    Render::DocumentRenderData data;
    generateElementEdgeRenderData(data);
    generateNodeRenderData(data);
    data.updateBoundingBox();
    return data;
}

void MeshDocumentImpl::generateElementEdgeRenderData(Render::DocumentRenderData& data) const {
    auto& batch = data.m_meshElementBatch;
    batch.m_primitiveType = Render::RenderPrimitiveType::Lines;

    constexpr float elem_color[4] = {0.2f, 0.8f, 0.3f, 1.0f};

    for(const auto& [eid, elem] : m_elements) {
        const uint8_t n = elem.nodeCount();
        if(n < 2) {
            continue;
        }

        const auto uid56 = elem.elementUID();
        const auto packed_uid =
            Render::RenderUID::encode(Render::RenderEntityType::MeshElement, uid56);

        const auto base_vertex = static_cast<uint32_t>(batch.m_vertices.size());
        const auto base_index = static_cast<uint32_t>(batch.m_indices.size());

        // Collect node positions
        double cx = 0.0, cy = 0.0, cz = 0.0;
        uint32_t valid_count = 0;

        for(uint8_t i = 0; i < n; ++i) {
            const MeshNodeId nid = elem.nodeId(i);
            const auto* node = findNodeById(nid);
            if(!node) {
                continue;
            }

            Render::RenderVertex v;
            v.m_position[0] = static_cast<float>(node->x());
            v.m_position[1] = static_cast<float>(node->y());
            v.m_position[2] = static_cast<float>(node->z());
            v.setColor(elem_color[0], elem_color[1], elem_color[2], elem_color[3]);
            v.setUid(packed_uid.m_packed);
            batch.m_vertices.push_back(v);
            batch.m_boundingBox.expand(node->position());

            cx += node->x();
            cy += node->y();
            cz += node->z();
            ++valid_count;
        }

        if(valid_count < 2) {
            // Rollback partially added vertices
            batch.m_vertices.resize(base_vertex);
            continue;
        }

        // Generate wireframe line indices based on element topology
        const auto elem_type = elem.elementType();
        if(elem_type == MeshElementType::Pyramid5 && valid_count == 5) {
            // Pyramid: quad base (0-1-2-3) + 4 apex edges (0-4, 1-4, 2-4, 3-4)
            for(uint32_t i = 0; i < 4; ++i) {
                batch.m_indices.push_back(base_vertex + i);
                batch.m_indices.push_back(base_vertex + ((i + 1) % 4));
            }
            for(uint32_t i = 0; i < 4; ++i) {
                batch.m_indices.push_back(base_vertex + i);
                batch.m_indices.push_back(base_vertex + 4);
            }
        } else if(elem_type == MeshElementType::Tetra4 && valid_count == 4) {
            // Tetrahedron: 6 edges (0-1, 0-2, 0-3, 1-2, 1-3, 2-3)
            constexpr uint32_t tet_edges[][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
            for(const auto& e : tet_edges) {
                batch.m_indices.push_back(base_vertex + e[0]);
                batch.m_indices.push_back(base_vertex + e[1]);
            }
        } else if(elem_type == MeshElementType::Hexa8 && valid_count == 8) {
            // Hexahedron: 12 edges
            constexpr uint32_t hex_edges[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0},
                                                  {4, 5}, {5, 6}, {6, 7}, {7, 4},
                                                  {0, 4}, {1, 5}, {2, 6}, {3, 7}};
            for(const auto& e : hex_edges) {
                batch.m_indices.push_back(base_vertex + e[0]);
                batch.m_indices.push_back(base_vertex + e[1]);
            }
        } else if(elem_type == MeshElementType::Prism6 && valid_count == 6) {
            // Prism: 9 edges (two triangular faces + 3 lateral edges)
            constexpr uint32_t prism_edges[][2] = {{0, 1}, {1, 2}, {2, 0},
                                                    {3, 4}, {4, 5}, {5, 3},
                                                    {0, 3}, {1, 4}, {2, 5}};
            for(const auto& e : prism_edges) {
                batch.m_indices.push_back(base_vertex + e[0]);
                batch.m_indices.push_back(base_vertex + e[1]);
            }
        } else {
            // Default: closed polygon (Line, Triangle, Quad4)
            for(uint32_t i = 0; i < valid_count; ++i) {
                batch.m_indices.push_back(base_vertex + i);
                batch.m_indices.push_back(base_vertex + ((i + 1) % valid_count));
            }
        }

        // Entity info
        Render::RenderEntityInfo info;
        info.m_uid = packed_uid;
        info.m_indexOffset = base_index;
        info.m_indexCount = static_cast<uint32_t>(batch.m_indices.size()) - base_index;
        info.m_vertexOffset = base_vertex;
        info.m_vertexCount = valid_count;

        info.m_hoverColor[0] = 1.0f;
        info.m_hoverColor[1] = 1.0f;
        info.m_hoverColor[2] = 0.0f;
        info.m_hoverColor[3] = 1.0f;

        info.m_selectedColor[0] = 1.0f;
        info.m_selectedColor[1] = 0.5f;
        info.m_selectedColor[2] = 0.0f;
        info.m_selectedColor[3] = 1.0f;

        const double inv_n = 1.0 / static_cast<double>(valid_count);
        info.m_centroid[0] = static_cast<float>(cx * inv_n);
        info.m_centroid[1] = static_cast<float>(cy * inv_n);
        info.m_centroid[2] = static_cast<float>(cz * inv_n);

        data.m_meshElementEntities[packed_uid.m_packed] = std::move(info);
    }
}

void MeshDocumentImpl::generateNodeRenderData(Render::DocumentRenderData& data) const {
    auto& batch = data.m_meshNodeBatch;
    batch.m_primitiveType = Render::RenderPrimitiveType::Points;

    for(const auto& [nid, node] : m_nodes) {
        const auto uid56 = nid;
        const auto packed_uid =
            Render::RenderUID::encode(Render::RenderEntityType::MeshNode, uid56);

        const auto base_vertex = static_cast<uint32_t>(batch.m_vertices.size());

        Render::RenderVertex v;
        v.m_position[0] = static_cast<float>(node.x());
        v.m_position[1] = static_cast<float>(node.y());
        v.m_position[2] = static_cast<float>(node.z());
        v.setColor(0.0f, 0.6f, 1.0f, 1.0f);
        v.setUid(packed_uid.m_packed);
        batch.m_vertices.push_back(v);
        batch.m_boundingBox.expand(node.position());

        Render::RenderEntityInfo info;
        info.m_uid = packed_uid;
        info.m_vertexOffset = base_vertex;
        info.m_vertexCount = 1;

        info.m_hoverColor[0] = 1.0f;
        info.m_hoverColor[1] = 1.0f;
        info.m_hoverColor[2] = 0.0f;
        info.m_hoverColor[3] = 1.0f;

        info.m_selectedColor[0] = 1.0f;
        info.m_selectedColor[1] = 0.5f;
        info.m_selectedColor[2] = 0.0f;
        info.m_selectedColor[3] = 1.0f;

        info.m_centroid[0] = static_cast<float>(node.x());
        info.m_centroid[1] = static_cast<float>(node.y());
        info.m_centroid[2] = static_cast<float>(node.z());

        data.m_meshNodeEntities[packed_uid.m_packed] = std::move(info);
    }
}

// =============================================================================
// Factory
// =============================================================================

MeshDocumentImplSingletonFactory::tObjectSharedPtr
MeshDocumentImplSingletonFactory::instance() const {
    return MeshDocumentImpl::instance();
}

} // namespace OpenGeoLab::Mesh
