/**
 * @file mesh_documentImpl.cpp
 * @brief Concrete implementation of MeshDocument
 */

#include "mesh_documentImpl.hpp"

#include "geometry/geometry_types.hpp"
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
    m_refToId.emplace(ref, id);
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
    LOG_DEBUG("MeshDocumentImpl: Cleared all nodes and elements");
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
    // Generate wireframe lines for each 2D element (Triangle, Quad4).
    // Each element gets its own RenderMesh so it can be independently picked.
    for(const auto& [eid, elem] : m_elements) {
        const uint8_t n = elem.nodeCount();
        if(n < 2) {
            continue;
        }

        Render::RenderMesh mesh;
        mesh.m_entityType = Geometry::EntityType::MeshElement;
        mesh.m_entityUid = static_cast<Geometry::EntityUID>(elem.elementUID());
        mesh.m_entityId = static_cast<Geometry::EntityId>(elem.elementId());
        mesh.m_primitiveType = Render::RenderPrimitiveType::Lines;

        // Wireframe color: light green for mesh elements.
        mesh.m_baseColor = Render::RenderColor{0.2f, 0.8f, 0.3f, 1.0f};
        mesh.m_hoverColor = Render::RenderColor{1.0f, 1.0f, 0.0f, 1.0f};
        mesh.m_selectedColor = Render::RenderColor{1.0f, 0.5f, 0.0f, 1.0f};

        // Collect node positions.
        std::vector<Render::RenderVertex> verts;
        verts.reserve(n);
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
            v.m_color = mesh.m_baseColor;
            mesh.m_boundingBox.expand(node->position());
            verts.push_back(v);
        }

        if(verts.size() < 2) {
            continue;
        }

        mesh.m_vertices = std::move(verts);

        // Generate line indices for wireframe: closed polygon edges.
        const auto vcount = static_cast<uint32_t>(mesh.m_vertices.size());
        for(uint32_t i = 0; i < vcount; ++i) {
            mesh.m_indices.push_back(i);
            mesh.m_indices.push_back((i + 1) % vcount);
        }

        data.m_meshElementMeshes.push_back(std::move(mesh));
    }
}

void MeshDocumentImpl::generateNodeRenderData(Render::DocumentRenderData& data) const {
    // Generate a single RenderMesh containing all mesh nodes as points.
    // Each node is a separate point vertex with its nodeId encoded as entityUid
    // for picking. We use one mesh per node to enable individual picking.

    for(const auto& [nid, node] : m_nodes) {
        Render::RenderMesh mesh;
        mesh.m_entityType = Geometry::EntityType::MeshNode;
        mesh.m_entityUid = static_cast<Geometry::EntityUID>(nid);
        mesh.m_primitiveType = Render::RenderPrimitiveType::Points;

        mesh.m_baseColor = Render::RenderColor{0.0f, 0.6f, 1.0f, 1.0f};
        mesh.m_hoverColor = Render::RenderColor{1.0f, 1.0f, 0.0f, 1.0f};
        mesh.m_selectedColor = Render::RenderColor{1.0f, 0.5f, 0.0f, 1.0f};

        Render::RenderVertex v;
        v.m_position[0] = static_cast<float>(node.x());
        v.m_position[1] = static_cast<float>(node.y());
        v.m_position[2] = static_cast<float>(node.z());
        v.m_color = mesh.m_baseColor;
        mesh.m_vertices.push_back(v);
        mesh.m_boundingBox.expand(node.position());

        data.m_meshNodeMeshes.push_back(std::move(mesh));
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
