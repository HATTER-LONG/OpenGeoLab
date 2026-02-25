#include "mesh_documentImpl.hpp"
#include "util/logger.hpp"

#include <unordered_map>

namespace OpenGeoLab::Mesh {
std::shared_ptr<MeshDocumentImpl> MeshDocumentImpl::instance() {
    static auto inst = std::shared_ptr<MeshDocumentImpl>(new MeshDocumentImpl());
    return inst;
}

// =============================================================================
// Node Management
// =============================================================================
bool MeshDocumentImpl::addNode(MeshNode node) {
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
    const MeshElementId id = element.elementId();
    if(id == INVALID_MESH_ELEMENT_ID || element.elementType() == MeshElementType::None ||
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
    {
        std::lock_guard<std::mutex> lock(m_renderDataMutex);
        m_renderDataValid = false;
    }
    LOG_DEBUG("MeshDocumentImpl: Notifying change, nodes={}, elements={}", m_nodes.size(),
              m_elements.size());
    m_changeSignal.emitSignal();
}

// =============================================================================
// Render Data
// =============================================================================

const Render::RenderData& MeshDocumentImpl::getRenderData() {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);
    if(m_renderDataValid) {
        return m_cachedRenderData;
    }

    m_cachedRenderData.clear();

    std::unordered_map<MeshNodeId, Util::Pt3d> node_lookup;
    node_lookup.reserve(m_nodes.size());
    for(const auto& node : m_nodes) {
        node_lookup.emplace(node.nodeId(), node.position());
    }

    for(const auto& element : m_elements) {
        const auto node_ids = element.nodeIds();
        if(node_ids.empty()) {
            continue;
        }

        Render::RenderPrimitive primitive;
        primitive.m_pass = Render::RenderPassType::Mesh;
        primitive.m_entityType = Render::toRenderEntityType(element.elementType());
        primitive.m_color = Render::RenderColor{0.17f, 0.63f, 0.94f, 1.0f};

        auto append_node = [&](MeshNodeId node_id) -> bool {
            const auto it = node_lookup.find(node_id);
            if(it == node_lookup.end()) {
                return false;
            }
            primitive.m_positions.push_back(static_cast<float>(it->second.x));
            primitive.m_positions.push_back(static_cast<float>(it->second.y));
            primitive.m_positions.push_back(static_cast<float>(it->second.z));
            return true;
        };

        switch(element.elementType()) {
        case MeshElementType::Line:
            primitive.m_topology = Render::PrimitiveTopology::Lines;
            if(node_ids.size() < 2 || !append_node(node_ids[0]) || !append_node(node_ids[1])) {
                continue;
            }
            primitive.m_indices = {0, 1};
            break;

        case MeshElementType::Triangle:
            primitive.m_topology = Render::PrimitiveTopology::Triangles;
            if(node_ids.size() < 3 || !append_node(node_ids[0]) || !append_node(node_ids[1]) ||
               !append_node(node_ids[2])) {
                continue;
            }
            primitive.m_indices = {0, 1, 2};
            break;

        case MeshElementType::Quad4:
            primitive.m_topology = Render::PrimitiveTopology::Triangles;
            if(node_ids.size() < 4 || !append_node(node_ids[0]) || !append_node(node_ids[1]) ||
               !append_node(node_ids[2]) || !append_node(node_ids[3])) {
                continue;
            }
            primitive.m_indices = {0, 1, 2, 0, 2, 3};
            break;

        default:
            continue;
        }

        if(!primitive.empty()) {
            m_cachedRenderData.m_primitives.push_back(std::move(primitive));
        }
    }

    m_renderDataValid = true;
    return m_cachedRenderData;
}

// =============================================================================
// Factory
// =============================================================================
MeshDocumentImplSingletonFactory::tObjectSharedPtr
MeshDocumentImplSingletonFactory::instance() const {
    return MeshDocumentImpl::instance();
}
} // namespace OpenGeoLab::Mesh