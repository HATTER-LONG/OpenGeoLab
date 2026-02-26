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
    m_nodeToLines.clear();
    m_nodeToElements.clear();
    m_lineToElements.clear();
    m_elementToLines.clear();
    m_edgeKeyToLineRef.clear();
    LOG_DEBUG("MeshDocumentImpl: Cleared all nodes, elements, and relations");
    notifyChanged();
}

// =============================================================================
// Edge Element Construction & Relation Maps
// =============================================================================

namespace {

/// Generate a composite key from a sorted pair of node IDs for edge deduplication.
uint64_t makeEdgeKey(MeshNodeId a, MeshNodeId b) {
    const auto lo = std::min(a, b);
    const auto hi = std::max(a, b);
    return (lo << 32u) | (hi & 0xFFFFFFFFu);
}

/// Edge tables for extracting edges from 2D/3D elements.
constexpr int TRIANGLE_EDGES[][2] = {{0, 1}, {1, 2}, {2, 0}};
constexpr int QUAD4_EDGES[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
constexpr int TETRA4_EDGES[][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
constexpr int HEXA8_EDGES[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                  {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
constexpr int PRISM6_EDGES[][2] = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5},
                                   {5, 3}, {0, 3}, {1, 4}, {2, 5}};
constexpr int PYRAMID5_EDGES[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0},
                                     {0, 4}, {1, 4}, {2, 4}, {3, 4}};

/// Return an edge table (pointer + count) for a given element type.
struct EdgeTable {
    const int (*m_edges)[2];
    size_t m_count;
};

EdgeTable edgeTableForType(MeshElementType type) {
    switch(type) {
    case MeshElementType::Triangle:
        return {TRIANGLE_EDGES, 3};
    case MeshElementType::Quad4:
        return {QUAD4_EDGES, 4};
    case MeshElementType::Tetra4:
        return {TETRA4_EDGES, 6};
    case MeshElementType::Hexa8:
        return {HEXA8_EDGES, 12};
    case MeshElementType::Prism6:
        return {PRISM6_EDGES, 9};
    case MeshElementType::Pyramid5:
        return {PYRAMID5_EDGES, 8};
    default:
        return {nullptr, 0};
    }
}

} // namespace

void MeshDocumentImpl::buildEdgeElements() {
    // Clear previous relation data
    m_nodeToLines.clear();
    m_nodeToElements.clear();
    m_lineToElements.clear();
    m_elementToLines.clear();
    m_edgeKeyToLineRef.clear();

    createLineElementsFromEdges();
    buildRelationMaps();

    LOG_DEBUG("MeshDocumentImpl::buildEdgeElements: {} Line elements, total elements {}",
              m_edgeKeyToLineRef.size(), m_elements.size());
}

/**
 * @brief Create Line elements for each unique edge found in 2D/3D elements.
 *
 * Existing Line elements (e.g., from Gmsh 1D meshing) are indexed first
 * to avoid duplication.
 */
void MeshDocumentImpl::createLineElementsFromEdges() {
    // Index existing Line elements by their node pair
    for(const auto& elem : m_elements) {
        if(elem.elementType() == MeshElementType::Line && elem.isValid()) {
            const uint64_t key = makeEdgeKey(elem.nodeId(0), elem.nodeId(1));
            m_edgeKeyToLineRef.emplace(key, elem.elementRef());
        }
    }

    // Scan non-Line elements and create new Line elements for unique edges
    const size_t original_count = m_elements.size();
    for(size_t ei = 0; ei < original_count; ++ei) {
        const auto& elem = m_elements[ei];
        if(!elem.isValid() || elem.elementType() == MeshElementType::Line ||
           elem.elementType() == MeshElementType::Node) {
            continue;
        }

        const auto [edges, count] = edgeTableForType(elem.elementType());
        if(!edges) {
            continue;
        }

        for(size_t i = 0; i < count; ++i) {
            const MeshNodeId n0 = elem.nodeId(edges[i][0]);
            const MeshNodeId n1 = elem.nodeId(edges[i][1]);
            if(n0 == INVALID_MESH_NODE_ID || n1 == INVALID_MESH_NODE_ID) {
                continue;
            }

            const uint64_t key = makeEdgeKey(n0, n1);
            if(m_edgeKeyToLineRef.find(key) == m_edgeKeyToLineRef.end()) {
                MeshElement line(MeshElementType::Line);
                line.setNodeId(0, std::min(n0, n1));
                line.setNodeId(1, std::max(n0, n1));
                m_edgeKeyToLineRef.emplace(key, line.elementRef());
                addElement(std::move(line));
            }
        }
    }
}

/**
 * @brief Build node↔line↔element relation maps from current elements.
 *
 * Populates: m_nodeToLines, m_nodeToElements, m_lineToElements, m_elementToLines.
 */
void MeshDocumentImpl::buildRelationMaps() {
    for(const auto& elem : m_elements) {
        if(!elem.isValid()) {
            continue;
        }

        const auto ref = elem.elementRef();

        if(elem.elementType() == MeshElementType::Line) {
            m_nodeToLines[elem.nodeId(0)].push_back(ref);
            m_nodeToLines[elem.nodeId(1)].push_back(ref);
        } else if(elem.elementType() != MeshElementType::Node) {
            // node → elements (non-Line)
            for(uint8_t i = 0; i < elem.nodeCount(); ++i) {
                const MeshNodeId nid = elem.nodeId(i);
                if(nid != INVALID_MESH_NODE_ID) {
                    m_nodeToElements[nid].push_back(ref);
                }
            }

            // element ↔ lines bidirectional linking
            const auto [edges, count] = edgeTableForType(elem.elementType());
            if(edges) {
                for(size_t i = 0; i < count; ++i) {
                    const MeshNodeId n0 = elem.nodeId(edges[i][0]);
                    const MeshNodeId n1 = elem.nodeId(edges[i][1]);
                    const uint64_t key = makeEdgeKey(n0, n1);
                    auto it = m_edgeKeyToLineRef.find(key);
                    if(it != m_edgeKeyToLineRef.end()) {
                        m_elementToLines[ref].push_back(it->second);
                        m_lineToElements[it->second].push_back(ref);
                    }
                }
            }
        }
    }
}

// =============================================================================
// Relation Queries
// =============================================================================

std::vector<MeshElementRef> MeshDocumentImpl::findLinesByNodeId(MeshNodeId node_id) const {
    auto it = m_nodeToLines.find(node_id);
    if(it != m_nodeToLines.end()) {
        return it->second;
    }
    return {};
}

std::vector<MeshElementRef> MeshDocumentImpl::findElementsByNodeId(MeshNodeId node_id) const {
    auto it = m_nodeToElements.find(node_id);
    if(it != m_nodeToElements.end()) {
        return it->second;
    }
    return {};
}

std::vector<MeshElementRef>
MeshDocumentImpl::findElementsByLineRef(const MeshElementRef& line_ref) const {
    auto it = m_lineToElements.find(line_ref);
    if(it != m_lineToElements.end()) {
        return it->second;
    }
    return {};
}

std::vector<MeshElementRef>
MeshDocumentImpl::findLinesByElementRef(const MeshElementRef& element_ref) const {
    auto it = m_elementToLines.find(element_ref);
    if(it != m_elementToLines.end()) {
        return it->second;
    }
    return {};
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
    Render::MeshRenderInput input{m_nodes, m_elements};
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