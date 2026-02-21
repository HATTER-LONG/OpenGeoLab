#include "mesh_documentImpl.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Mesh {
std::shared_ptr<MeshDocumentImpl> MeshDocumentImpl::instance() {
    static std::shared_ptr<MeshDocumentImpl> instance = std::make_shared<MeshDocumentImpl>();
    return instance;
}

// =============================================================================
// Node Management
// =============================================================================
bool MeshDocumentImpl::addNode(MeshNode node) {
    m_renderDataDirty = true;
    const MeshNodeId id = node.nodeId();
    if(id == INVALID_MESH_NODE_ID || m_nodes.size() != id) {
        return false;
    }

    m_nodes.emplace_back(std::move(node));
    return true;
}

MeshNode MeshDocumentImpl::findNodeById(MeshNodeId node_id) const {
    if(node_id == INVALID_MESH_NODE_ID || node_id >= m_nodes.size() ||
       m_nodes[node_id].nodeId() == INVALID_MESH_NODE_ID) {
        throw std::out_of_range("Mesh node id not found: " + std::to_string(node_id));
    }
    return m_nodes[node_id];
}

size_t MeshDocumentImpl::nodeCount() const { return m_nodes.size(); }

// =============================================================================
// Element Management
// =============================================================================

bool MeshDocumentImpl::addElement(MeshElement element) {
    m_renderDataDirty = true;
    const MeshElementId id = element.elementId();
    if(id == INVALID_MESH_ELEMENT_ID || element.elementType() == MeshElementType::Invalid ||
       id != m_elements.size()) {
        return false;
    }

    m_refToId.emplace(element.elementRef(), id);
    m_elements.emplace_back(std::move(element));
    return true;
}

MeshElement MeshDocumentImpl::findElementById(MeshElementId element_id) const {
    if(element_id == INVALID_MESH_ELEMENT_ID || element_id >= m_elements.size() ||
       m_elements[element_id].elementId() == INVALID_MESH_ELEMENT_ID) {
        throw std::out_of_range("Mesh element id not found: " + std::to_string(element_id));
    }
    return m_elements[element_id];
}

MeshElement MeshDocumentImpl::findElementByRef(const MeshElementRef& ref) const {
    auto it = m_refToId.find(ref);
    if(it == m_refToId.end()) {
        throw std::out_of_range("Mesh element not found for ref: " + std::to_string(ref.m_uid));
    }
    return m_elements[it->second];
}

size_t MeshDocumentImpl::elementCount() const { return m_elements.size(); }

void MeshDocumentImpl::clear() {
    m_nodes.clear();
    m_elements.clear();
    m_refToId.clear();
    LOG_DEBUG("MeshDocumentImpl: Cleared all nodes and elements");
    notifyChanged();
}

// =============================================================================
// Change Notification
// =============================================================================

Util::ScopedConnection MeshDocumentImpl::subscribeToChanges(std::function<void()> callback) {
    return m_changeSignal.connect(std::move(callback));
}

void MeshDocumentImpl::notifyChanged() {
    LOG_DEBUG("MeshDocumentImpl: Notifying change, nodes={}, elements={}", m_nodes.size(),
              m_elements.size());
    m_changeSignal.emitSignal();
}

// =============================================================================
// Render Data
// =============================================================================

const Render::DocumentRenderData& MeshDocumentImpl::getRenderData() {
    if(!m_renderDataDirty) {
        return m_renderData;
    }
    generateElementEdgeRenderData();
    generateNodeRenderData();
    m_renderData.updateBoundingBox();
    m_renderDataDirty = false;
    return m_renderData;
}

void MeshDocumentImpl::generateElementEdgeRenderData() {
    try {

        for(const auto& element : m_elements) {

            const auto node_ids = element.nodeIds();
            const size_t n = node_ids.size();
            if(!element.isValid() || node_ids.size() < 2 || node_ids.size() > 8) {
                continue;
            }

            Render::MeshRenderData mesh;
            mesh.m_pickType = Render::PickEntityType::MeshElement;
            mesh.m_elementId = element.elementId();
            mesh.m_elementUid = element.elementUID();
            mesh.m_primitiveType = Render::RenderPrimitiveType::Lines;

            // Wireframe color: light green for mesh elements.
            mesh.m_baseColor = Render::RenderColor{0.2f, 0.8f, 0.3f, 1.0f};
            mesh.m_hoverColor = Render::RenderColor{1.0f, 1.0f, 0.0f, 1.0f};
            mesh.m_selectedColor = Render::RenderColor{1.0f, 0.5f, 0.0f, 1.0f};

            // Collect node positions.
            std::vector<Render::RenderVertex> verts;
            verts.reserve(n);
            for(uint8_t i = 0; i < n; ++i) {
                const MeshNodeId nid = element.nodeId(i);
                auto node = findNodeById(nid);
                Render::RenderVertex v;
                v.m_position[0] = static_cast<float>(node.x());
                v.m_position[1] = static_cast<float>(node.y());
                v.m_position[2] = static_cast<float>(node.z());
                v.m_color = mesh.m_baseColor;
                mesh.m_boundingBox.expand(node.position());
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
            m_renderData.m_meshElementMeshes.emplace_back(std::move(mesh));
        }
    } catch(const std::exception& e) {
        LOG_ERROR("MeshDocumentImpl: Exception in generateElementEdgeRenderData: {}", e.what());
        throw e;
    }
}

void MeshDocumentImpl::generateNodeRenderData() {
    m_renderData.m_meshNodeMeshes.clear();
    for(const auto& node : m_nodes) {
        if(node.nodeId() == INVALID_MESH_NODE_ID) {
            continue;
        }

        Render::MeshRenderData mesh;
        mesh.m_pickType = Render::PickEntityType::MeshNode;
        mesh.m_nodeId = node.nodeId();
        mesh.m_primitiveType = Render::RenderPrimitiveType::Points;

        // Node color: cyan for visibility
        mesh.m_baseColor = Render::RenderColor{0.0f, 1.0f, 1.0f, 1.0f};
        mesh.m_hoverColor = Render::RenderColor{1.0f, 1.0f, 0.0f, 1.0f};
        mesh.m_selectedColor = Render::RenderColor{1.0f, 0.5f, 0.0f, 1.0f};

        Render::RenderVertex v;
        v.m_position[0] = static_cast<float>(node.x());
        v.m_position[1] = static_cast<float>(node.y());
        v.m_position[2] = static_cast<float>(node.z());
        v.m_color = mesh.m_baseColor;
        mesh.m_boundingBox.expand(node.position());
        mesh.m_vertices.push_back(v);

        m_renderData.m_meshNodeMeshes.emplace_back(std::move(mesh));
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