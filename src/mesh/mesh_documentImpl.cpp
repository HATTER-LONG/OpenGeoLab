#include "mesh_documentImpl.hpp"
#include "util/logger.hpp"

#include <unordered_map>

namespace {
struct MeshBatch {
    OpenGeoLab::Render::RenderPrimitive m_primitive;
    std::unordered_map<OpenGeoLab::Mesh::MeshNodeId, uint32_t> m_localNodeToIndex;
};

[[nodiscard]] const OpenGeoLab::Mesh::MeshNode*
findMeshNodeById(const std::vector<OpenGeoLab::Mesh::MeshNode>& nodes,
                 OpenGeoLab::Mesh::MeshNodeId node_id) {
    if(node_id == OpenGeoLab::Mesh::INVALID_MESH_NODE_ID || node_id > nodes.size()) {
        return nullptr;
    }

    const auto index = static_cast<size_t>(node_id - 1);
    if(nodes[index].nodeId() != node_id) {
        return nullptr;
    }
    return &nodes[index];
}

[[nodiscard]] OpenGeoLab::Render::PrimitiveTopology
topologyForMeshType(OpenGeoLab::Mesh::MeshElementType type) {
    return type == OpenGeoLab::Mesh::MeshElementType::Line
               ? OpenGeoLab::Render::PrimitiveTopology::Lines
               : OpenGeoLab::Render::PrimitiveTopology::Triangles;
}

void appendTriangle(std::vector<uint32_t>& indices, uint32_t a, uint32_t b, uint32_t c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
}

void appendQuadAsTriangles(
    std::vector<uint32_t>& indices, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    appendTriangle(indices, a, b, c);
    appendTriangle(indices, a, c, d);
}

void appendElementIndices(OpenGeoLab::Mesh::MeshElementType mesh_type,
                          const std::vector<uint32_t>& local_indices,
                          std::vector<uint32_t>& out_indices) {
    using OpenGeoLab::Mesh::MeshElementType;

    switch(mesh_type) {
    case MeshElementType::Line:
        if(local_indices.size() == 2) {
            out_indices.push_back(local_indices[0]);
            out_indices.push_back(local_indices[1]);
        }
        break;
    case MeshElementType::Triangle:
        if(local_indices.size() == 3) {
            appendTriangle(out_indices, local_indices[0], local_indices[1], local_indices[2]);
        }
        break;
    case MeshElementType::Quad4:
        if(local_indices.size() == 4) {
            appendQuadAsTriangles(out_indices, local_indices[0], local_indices[1], local_indices[2],
                                  local_indices[3]);
        }
        break;
    case MeshElementType::Tetra4:
        if(local_indices.size() == 4) {
            appendTriangle(out_indices, local_indices[0], local_indices[2], local_indices[1]);
            appendTriangle(out_indices, local_indices[0], local_indices[1], local_indices[3]);
            appendTriangle(out_indices, local_indices[1], local_indices[2], local_indices[3]);
            appendTriangle(out_indices, local_indices[2], local_indices[0], local_indices[3]);
        }
        break;
    case MeshElementType::Hexa8:
        if(local_indices.size() == 8) {
            appendQuadAsTriangles(out_indices, local_indices[0], local_indices[1], local_indices[2],
                                  local_indices[3]);
            appendQuadAsTriangles(out_indices, local_indices[4], local_indices[7], local_indices[6],
                                  local_indices[5]);
            appendQuadAsTriangles(out_indices, local_indices[0], local_indices[4], local_indices[5],
                                  local_indices[1]);
            appendQuadAsTriangles(out_indices, local_indices[1], local_indices[5], local_indices[6],
                                  local_indices[2]);
            appendQuadAsTriangles(out_indices, local_indices[2], local_indices[6], local_indices[7],
                                  local_indices[3]);
            appendQuadAsTriangles(out_indices, local_indices[3], local_indices[7], local_indices[4],
                                  local_indices[0]);
        }
        break;
    case MeshElementType::Prism6:
        if(local_indices.size() == 6) {
            appendTriangle(out_indices, local_indices[0], local_indices[1], local_indices[2]);
            appendTriangle(out_indices, local_indices[3], local_indices[5], local_indices[4]);
            appendQuadAsTriangles(out_indices, local_indices[0], local_indices[1], local_indices[4],
                                  local_indices[3]);
            appendQuadAsTriangles(out_indices, local_indices[1], local_indices[2], local_indices[5],
                                  local_indices[4]);
            appendQuadAsTriangles(out_indices, local_indices[2], local_indices[0], local_indices[3],
                                  local_indices[5]);
        }
        break;
    case MeshElementType::Pyramid5:
        if(local_indices.size() == 5) {
            appendQuadAsTriangles(out_indices, local_indices[0], local_indices[1], local_indices[2],
                                  local_indices[3]);
            appendTriangle(out_indices, local_indices[0], local_indices[1], local_indices[4]);
            appendTriangle(out_indices, local_indices[1], local_indices[2], local_indices[4]);
            appendTriangle(out_indices, local_indices[2], local_indices[3], local_indices[4]);
            appendTriangle(out_indices, local_indices[3], local_indices[0], local_indices[4]);
        }
        break;
    default:
        break;
    }
}
} // namespace

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

bool MeshDocumentImpl::getRenderData(Render::RenderData& render_data) {
    std::lock_guard<std::mutex> lock(m_renderDataMutex);

    render_data.m_mesh.clear();
    if(m_nodes.empty() || m_elements.empty()) {
        return true;
    }

    std::unordered_map<Render::RenderEntityType, MeshBatch> batches;
    batches.reserve(7);

    for(const auto& element : m_elements) {
        const auto mesh_type = element.elementType();
        const auto entity_type = Render::toRenderEntityType(mesh_type);
        if(entity_type == Render::RenderEntityType::None || mesh_type == MeshElementType::Node) {
            continue;
        }

        auto [batch_it, inserted] = batches.try_emplace(entity_type);
        auto& batch = batch_it->second;
        if(inserted) {
            batch.m_primitive.m_entityType = entity_type;
            batch.m_primitive.m_topology = topologyForMeshType(mesh_type);
            batch.m_primitive.m_passType = Render::RenderPassType::Mesh;
        }
        if(batch.m_primitive.m_uid == 0) {
            batch.m_primitive.m_uid = element.elementUID();
        }

        std::vector<uint32_t> local_indices;
        local_indices.reserve(element.nodeCount());

        bool valid_element = true;
        for(uint8_t i = 0; i < element.nodeCount(); ++i) {
            const auto node_id = element.nodeId(i);
            auto local_it = batch.m_localNodeToIndex.find(node_id);
            if(local_it == batch.m_localNodeToIndex.end()) {
                const auto* node = findMeshNodeById(m_nodes, node_id);
                if(node == nullptr) {
                    valid_element = false;
                    break;
                }

                const auto local_index =
                    static_cast<uint32_t>(batch.m_primitive.m_positions.size());
                batch.m_primitive.m_positions.push_back(node->position());
                local_it = batch.m_localNodeToIndex.emplace(node_id, local_index).first;
            }
            local_indices.push_back(local_it->second);
        }

        if(!valid_element) {
            continue;
        }

        appendElementIndices(mesh_type, local_indices, batch.m_primitive.m_indices);
    }

    render_data.m_mesh.reserve(batches.size());
    for(auto& [entity_type, batch] : batches) {
        (void)entity_type;
        if(batch.m_primitive.isValid()) {
            render_data.m_mesh.emplace_back(std::move(batch.m_primitive));
        }
    }

    return true;
}

// =============================================================================
// Factory
// =============================================================================
MeshDocumentImplSingletonFactory::tObjectSharedPtr
MeshDocumentImplSingletonFactory::instance() const {
    return MeshDocumentImpl::instance();
}
} // namespace OpenGeoLab::Mesh