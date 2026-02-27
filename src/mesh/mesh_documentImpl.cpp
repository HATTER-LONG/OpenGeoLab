/**
 * @file mesh_documentImpl.cpp
 * @brief MeshDocumentImpl singleton — thread-safe mesh storage and render
 *        data generation via MeshRenderBuilder.
 */

#include "mesh_documentImpl.hpp"
#include "render/builder/mesh_render_builder.hpp"
#include "util/logger.hpp"

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
    if(id == INVALID_MESH_NODE_ID || id != m_nodes.size() + 1) {
        return false;
    }

    m_nodes.emplace_back(std::move(node));
    return true;
}

MeshNode MeshDocumentImpl::findNodeById(MeshNodeId node_id) const {
    if(node_id == INVALID_MESH_NODE_ID || node_id > m_nodes.size()) {
        throw std::out_of_range("Mesh node id not found: " + std::to_string(node_id));
    }

    const auto index = static_cast<size_t>(node_id - 1);
    if(m_nodes[index].nodeId() == INVALID_MESH_NODE_ID) {
        throw std::out_of_range("Mesh node id not found: " + std::to_string(node_id));
    }
    return m_nodes[index];
}

size_t MeshDocumentImpl::nodeCount() const { return m_nodes.size(); }

// =============================================================================
// Element Management
// =============================================================================

bool MeshDocumentImpl::addElement(MeshElement element) {
    const MeshElementId id = element.elementId();
    if(id == INVALID_MESH_ELEMENT_ID || element.elementType() == MeshElementType::None ||
       id != m_elements.size() + 1) {
        return false;
    }

    m_refToId.emplace(element.elementRef(), static_cast<MeshElementId>(m_elements.size()));
    m_elements.emplace_back(std::move(element));
    return true;
}

MeshElement MeshDocumentImpl::findElementById(MeshElementId element_id) const {
    if(element_id == INVALID_MESH_ELEMENT_ID || element_id > m_elements.size()) {
        throw std::out_of_range("Mesh element id not found: " + std::to_string(element_id));
    }

    const auto index = static_cast<size_t>(element_id - 1);
    if(m_elements[index].elementId() == INVALID_MESH_ELEMENT_ID) {
        throw std::out_of_range("Mesh element id not found: " + std::to_string(element_id));
    }
    return m_elements[index];
}

MeshElement MeshDocumentImpl::findElementByRef(const MeshElementRef& ref) const {
    auto it = m_refToId.find(ref);
    if(it == m_refToId.end()) {
        throw std::out_of_range("Mesh element not found for ref: " + std::to_string(ref.m_uid));
    }
    if(it->second >= m_elements.size()) {
        throw std::out_of_range("Mesh element index out of range for ref: " +
                                std::to_string(ref.m_uid));
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

bool MeshDocumentImpl::getRenderData(Render::RenderData& render_data,
                                     const Render::RenderColor& surface_color) {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);
    Render::MeshRenderInput input{m_nodes, m_elements, surface_color};
    return Render::MeshRenderBuilder::build(render_data, input);
}

// =============================================================================
// Factory
// =============================================================================
MeshDocumentImplSingletonFactory::tObjectSharedPtr
MeshDocumentImplSingletonFactory::instance() const {
    return MeshDocumentImpl::instance();
}
} // namespace OpenGeoLab::Mesh