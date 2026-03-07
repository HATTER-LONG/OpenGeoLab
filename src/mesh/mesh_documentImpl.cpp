/**
 * @file mesh_documentImpl.cpp
 * @brief MeshDocumentImpl singleton — thread-safe mesh storage and render
 *        data generation via MeshRenderBuilder.
 */
#include "mesh_documentImpl.hpp"
#include "mesh_render_builder.hpp"
#include "util/logger.hpp"

#include <mutex>

namespace OpenGeoLab::Mesh {
std::shared_ptr<MeshDocumentImpl> MeshDocumentImpl::instance() {
    static auto inst = std::shared_ptr<MeshDocumentImpl>(new MeshDocumentImpl());
    return inst;
}

// =============================================================================
// Node Management
// =============================================================================
bool MeshDocumentImpl::addNode(MeshNode node) {
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    return addNodeUnlocked(std::move(node));
}

bool MeshDocumentImpl::addNodeUnlocked(MeshNode node) {
    const MeshNodeId id = node.nodeId();
    if(id == INVALID_MESH_NODE_ID || m_nodeIdToIndex.find(id) != m_nodeIdToIndex.end()) {
        return false;
    }

    m_nodes.emplace_back(std::move(node));
    m_nodeIdToIndex.emplace(id, m_nodes.size() - 1);
    return true;
}

void MeshDocumentImpl::reserveNodeCapacity(size_t capacity) {
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    reserveNodeCapacityUnlocked(capacity);
}

void MeshDocumentImpl::reserveNodeCapacityUnlocked(size_t capacity) {
    m_nodes.reserve(capacity);
    m_nodeIdToIndex.reserve(capacity);
}

MeshNode MeshDocumentImpl::findNodeById(MeshNodeId node_id) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findNodeByIdUnlocked(node_id);
}

MeshNode MeshDocumentImpl::findNodeByIdUnlocked(MeshNodeId node_id) const {
    if(node_id == INVALID_MESH_NODE_ID) {
        throw std::out_of_range("Mesh node id not found: " + std::to_string(node_id));
    }

    const auto it = m_nodeIdToIndex.find(node_id);
    if(it == m_nodeIdToIndex.end()) {
        throw std::out_of_range("Mesh node id not found: " + std::to_string(node_id));
    }
    return m_nodes[it->second];
}

size_t MeshDocumentImpl::nodeCount() const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return nodeCountUnlocked();
}

size_t MeshDocumentImpl::nodeCountUnlocked() const { return m_nodeIdToIndex.size(); }

// =============================================================================
// Element Management
// =============================================================================

bool MeshDocumentImpl::addElement(MeshElement element) {
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    return addElementUnlocked(std::move(element));
}

bool MeshDocumentImpl::addElementUnlocked(MeshElement element) {
    const MeshElementId id = element.elementId();
    const MeshElementRef ref = element.elementRef();
    if(id == INVALID_MESH_ELEMENT_ID || element.elementType() == MeshElementType::None ||
       m_elementIdToIndex.find(id) != m_elementIdToIndex.end() ||
       m_refToIndex.find(ref) != m_refToIndex.end()) {
        return false;
    }

    m_elements.emplace_back(std::move(element));
    const size_t index = m_elements.size() - 1;
    m_elementIdToIndex.emplace(id, index);
    m_refToIndex.emplace(ref, index);
    return true;
}

void MeshDocumentImpl::reserveElementCapacity(size_t capacity) {
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    reserveElementCapacityUnlocked(capacity);
}

void MeshDocumentImpl::reserveElementCapacityUnlocked(size_t capacity) {
    m_elements.reserve(capacity);
    m_elementIdToIndex.reserve(capacity);
    m_refToIndex.reserve(capacity);
}

MeshElement MeshDocumentImpl::findElementById(MeshElementId element_id) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findElementByIdUnlocked(element_id);
}

MeshElement MeshDocumentImpl::findElementByIdUnlocked(MeshElementId element_id) const {
    if(element_id == INVALID_MESH_ELEMENT_ID) {
        throw std::out_of_range("Mesh element id not found: " + std::to_string(element_id));
    }

    const auto it = m_elementIdToIndex.find(element_id);
    if(it == m_elementIdToIndex.end()) {
        throw std::out_of_range("Mesh element id not found: " + std::to_string(element_id));
    }
    return m_elements[it->second];
}

MeshElement MeshDocumentImpl::findElementByRef(const MeshElementRef& ref) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findElementByRefUnlocked(ref);
}

MeshElement MeshDocumentImpl::findElementByRefUnlocked(const MeshElementRef& ref) const {
    auto it = m_refToIndex.find(ref);
    if(it == m_refToIndex.end()) {
        throw std::out_of_range("Mesh element not found for ref: " + std::to_string(ref.m_uid));
    }
    if(it->second >= m_elements.size()) {
        throw std::out_of_range("Mesh element index out of range for ref: " +
                                std::to_string(ref.m_uid));
    }
    return m_elements[it->second];
}

size_t MeshDocumentImpl::elementCount() const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return elementCountUnlocked();
}

size_t MeshDocumentImpl::elementCountUnlocked() const { return m_elementIdToIndex.size(); }

void MeshDocumentImpl::clear() {
    size_t node_count = 0;
    size_t element_count = 0;
    {
        std::unique_lock<std::shared_mutex> lock(m_documentMutex);
        node_count = nodeCountUnlocked();
        element_count = elementCountUnlocked();
        clearUnlocked();
    }
    LOG_DEBUG("MeshDocumentImpl: Cleared {} nodes, {} elements, reset mesh id generators",
              node_count, element_count);
    notifyChanged();
}

bool MeshDocumentImpl::replaceMeshData(std::vector<MeshNode> nodes,
                                       std::vector<MeshElement> elements,
                                       std::string& error) {
    std::unordered_map<MeshNodeId, size_t> node_id_to_index;
    node_id_to_index.reserve(nodes.size());
    for(size_t index = 0; index < nodes.size(); ++index) {
        const MeshNodeId node_id = nodes[index].nodeId();
        if(node_id == INVALID_MESH_NODE_ID) {
            error = "Generated mesh contains an invalid node id";
            return false;
        }
        if(!node_id_to_index.emplace(node_id, index).second) {
            error = "Generated mesh contains duplicate node ids";
            return false;
        }
    }

    std::unordered_map<MeshElementId, size_t> element_id_to_index;
    element_id_to_index.reserve(elements.size());
    std::unordered_map<MeshElementRef, size_t, MeshElementRefHash> ref_to_index;
    ref_to_index.reserve(elements.size());
    for(size_t index = 0; index < elements.size(); ++index) {
        const auto& element = elements[index];
        if(element.elementId() == INVALID_MESH_ELEMENT_ID ||
           element.elementType() == MeshElementType::None) {
            error = "Generated mesh contains an invalid element";
            return false;
        }
        if(!element_id_to_index.emplace(element.elementId(), index).second) {
            error = "Generated mesh contains duplicate element ids";
            return false;
        }
        if(!ref_to_index.emplace(element.elementRef(), index).second) {
            error = "Generated mesh contains duplicate element references";
            return false;
        }
    }

    {
        std::unique_lock<std::shared_mutex> lock(m_documentMutex);
        m_nodes = std::move(nodes);
        m_elements = std::move(elements);
        m_nodeIdToIndex = std::move(node_id_to_index);
        m_elementIdToIndex = std::move(element_id_to_index);
        m_refToIndex = std::move(ref_to_index);
        m_nodeToLines.clear();
        m_nodeToElements.clear();
        m_lineToElements.clear();
        m_elementToLines.clear();
        m_edgeKeyToLineRef.clear();
        createLineElementsFromEdges();
        buildRelationMaps();
    }

    notifyChanged();
    return true;
}

bool MeshDocumentImpl::appendMeshData(std::vector<MeshNode> nodes,
                                      std::vector<MeshElement> elements,
                                      std::string& error) {
    std::unordered_map<MeshNodeId, size_t> incoming_node_id_to_index;
    incoming_node_id_to_index.reserve(nodes.size());
    for(size_t index = 0; index < nodes.size(); ++index) {
        const MeshNodeId node_id = nodes[index].nodeId();
        if(node_id == INVALID_MESH_NODE_ID) {
            error = "Generated mesh contains an invalid node id";
            return false;
        }
        if(!incoming_node_id_to_index.emplace(node_id, index).second) {
            error = "Generated mesh contains duplicate node ids";
            return false;
        }
    }

    std::unordered_map<MeshElementId, size_t> incoming_element_id_to_index;
    incoming_element_id_to_index.reserve(elements.size());
    std::unordered_map<MeshElementRef, size_t, MeshElementRefHash> incoming_ref_to_index;
    incoming_ref_to_index.reserve(elements.size());
    for(size_t index = 0; index < elements.size(); ++index) {
        const auto& element = elements[index];
        if(element.elementId() == INVALID_MESH_ELEMENT_ID ||
           element.elementType() == MeshElementType::None) {
            error = "Generated mesh contains an invalid element";
            return false;
        }
        if(!incoming_element_id_to_index.emplace(element.elementId(), index).second) {
            error = "Generated mesh contains duplicate element ids";
            return false;
        }
        if(!incoming_ref_to_index.emplace(element.elementRef(), index).second) {
            error = "Generated mesh contains duplicate element references";
            return false;
        }
    }

    {
        std::unique_lock<std::shared_mutex> lock(m_documentMutex);

        for(const auto& node : nodes) {
            if(m_nodeIdToIndex.contains(node.nodeId())) {
                error = "Generated mesh node id conflicts with existing mesh data";
                return false;
            }
        }

        for(const auto& element : elements) {
            if(m_elementIdToIndex.contains(element.elementId())) {
                error = "Generated mesh element id conflicts with existing mesh data";
                return false;
            }
            if(m_refToIndex.contains(element.elementRef())) {
                error = "Generated mesh element reference conflicts with existing mesh data";
                return false;
            }
        }

        reserveNodeCapacityUnlocked(m_nodes.size() + nodes.size());
        reserveElementCapacityUnlocked(m_elements.size() + elements.size());

        for(auto& node : nodes) {
            const size_t index = m_nodes.size();
            m_nodeIdToIndex.emplace(node.nodeId(), index);
            m_nodes.emplace_back(std::move(node));
        }

        for(auto& element : elements) {
            const size_t index = m_elements.size();
            m_elementIdToIndex.emplace(element.elementId(), index);
            m_refToIndex.emplace(element.elementRef(), index);
            m_elements.emplace_back(std::move(element));
        }

        m_nodeToLines.clear();
        m_nodeToElements.clear();
        m_lineToElements.clear();
        m_elementToLines.clear();
        m_edgeKeyToLineRef.clear();
        createLineElementsFromEdges();
        buildRelationMaps();
    }

    notifyChanged();
    return true;
}

void MeshDocumentImpl::snapshotMesh(std::vector<MeshNode>& nodes,
                                    std::vector<MeshElement>& elements) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    nodes = m_nodes;
    elements = m_elements;
}

bool MeshDocumentImpl::updateNodePositions(
    const std::unordered_map<MeshNodeId, Util::Pt3d>& positions, std::string& error) {
    if(positions.empty()) {
        error = "No node positions provided";
        return false;
    }

    {
        std::unique_lock<std::shared_mutex> lock(m_documentMutex);
        for(const auto& [node_id, position] : positions) {
            const auto it = m_nodeIdToIndex.find(node_id);
            if(it == m_nodeIdToIndex.end()) {
                error = "Mesh node id not found: " + std::to_string(node_id);
                return false;
            }
            m_nodes[it->second].setPosition(position);
        }
    }

    notifyChanged();
    return true;
}

void MeshDocumentImpl::clearUnlocked() {
    m_nodes.clear();
    m_elements.clear();
    m_nodeIdToIndex.clear();
    m_elementIdToIndex.clear();
    m_refToIndex.clear();
    m_nodeToLines.clear();
    m_nodeToElements.clear();
    m_lineToElements.clear();
    m_elementToLines.clear();
    m_edgeKeyToLineRef.clear();
    resetMeshElementIdGenerator();
    resetAllMeshElementUIDGenerators();
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
    std::unique_lock<std::shared_mutex> lock(m_documentMutex);
    m_nodeToLines.clear();
    m_nodeToElements.clear();
    m_lineToElements.clear();
    m_elementToLines.clear();
    m_edgeKeyToLineRef.clear();

    createLineElementsFromEdges();
    buildRelationMaps();
    LOG_DEBUG("MeshDocumentImpl::buildEdgeElements: {} Line elements, total elements {}",
              m_edgeKeyToLineRef.size(), m_elementIdToIndex.size());
}

void MeshDocumentImpl::createLineElementsFromEdges() {
    // Index existing Line elements by their node pair
    for(const auto& elem : m_elements) {
        if(elem.elementType() == MeshElementType::Line && elem.isValid()) {
            const uint64_t key = makeEdgeKey(elem.nodeId(0), elem.nodeId(1));
            m_edgeKeyToLineRef.emplace(key, elem.elementRef());
        }
    }

    size_t estimated_new_lines = 0;
    std::unordered_map<uint64_t, uint64_t> edge_to_part_uid;
    for(const auto& elem : m_elements) {
        if(!elem.isValid() || elem.elementType() == MeshElementType::Line ||
           elem.elementType() == MeshElementType::Node) {
            continue;
        }

        const auto [edges, count] = edgeTableForType(elem.elementType());
        estimated_new_lines += count;

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
            auto [it, inserted] = edge_to_part_uid.emplace(key, elem.partUid());
            if(!inserted && it->second != elem.partUid()) {
                it->second = 0;
            }
        }
    }
    reserveElementCapacityUnlocked(m_elements.size() + estimated_new_lines);

    for(const auto& [key, line_ref] : m_edgeKeyToLineRef) {
        const auto owner_it = edge_to_part_uid.find(key);
        if(owner_it == edge_to_part_uid.end()) {
            continue;
        }

        const auto index_it = m_refToIndex.find(line_ref);
        if(index_it == m_refToIndex.end()) {
            continue;
        }

        m_elements[index_it->second].setPartUid(owner_it->second);
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
                const auto owner_it = edge_to_part_uid.find(key);
                if(owner_it != edge_to_part_uid.end()) {
                    line.setPartUid(owner_it->second);
                }
                const MeshElementRef line_ref = line.elementRef();
                if(addElementUnlocked(std::move(line))) {
                    m_edgeKeyToLineRef.emplace(key, line_ref);
                }
            }
        }
    }
}

void MeshDocumentImpl::buildRelationMaps() {
    for(const auto& elem : m_elements) {
        if(!elem.isValid()) {
            continue;
        }

        const auto ref = elem.elementRef();

        if(elem.elementType() == MeshElementType::Line) {
            if(elem.nodeId(0) != INVALID_MESH_NODE_ID) {
                m_nodeToLines[elem.nodeId(0)].push_back(ref);
            }
            if(elem.nodeId(1) != INVALID_MESH_NODE_ID) {
                m_nodeToLines[elem.nodeId(1)].push_back(ref);
            }
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
                    if(n0 == INVALID_MESH_NODE_ID || n1 == INVALID_MESH_NODE_ID) {
                        continue;
                    }
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
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findLinesByNodeIdUnlocked(node_id);
}

std::vector<MeshElementRef> MeshDocumentImpl::findLinesByNodeIdUnlocked(MeshNodeId node_id) const {
    auto it = m_nodeToLines.find(node_id);
    if(it != m_nodeToLines.end()) {
        return it->second;
    }
    return {};
}

std::vector<MeshElementRef> MeshDocumentImpl::findElementsByNodeId(MeshNodeId node_id) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findElementsByNodeIdUnlocked(node_id);
}

std::vector<MeshElementRef>
MeshDocumentImpl::findElementsByNodeIdUnlocked(MeshNodeId node_id) const {
    auto it = m_nodeToElements.find(node_id);
    if(it != m_nodeToElements.end()) {
        return it->second;
    }
    return {};
}

std::vector<MeshElementRef>
MeshDocumentImpl::findElementsByLineRef(const MeshElementRef& line_ref) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findElementsByLineRefUnlocked(line_ref);
}

std::vector<MeshElementRef>
MeshDocumentImpl::findElementsByLineRefUnlocked(const MeshElementRef& line_ref) const {
    auto it = m_lineToElements.find(line_ref);
    if(it != m_lineToElements.end()) {
        return it->second;
    }
    return {};
}

std::vector<MeshElementRef>
MeshDocumentImpl::findLinesByElementRef(const MeshElementRef& element_ref) const {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return findLinesByElementRefUnlocked(element_ref);
}

std::vector<MeshElementRef>
MeshDocumentImpl::findLinesByElementRefUnlocked(const MeshElementRef& element_ref) const {
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
    size_t node_count = 0;
    size_t element_count = 0;
    {
        std::shared_lock<std::shared_mutex> lock(m_documentMutex);
        node_count = m_nodes.size();
        element_count = m_elements.size();
    }
    LOG_DEBUG("MeshDocumentImpl: Notifying change, nodes={}, elements={}", node_count,
              element_count);
    m_changeSignal.emitSignal();
}

// =============================================================================
// Render Data
// =============================================================================

bool MeshDocumentImpl::getRenderData(Render::RenderData& render_data) {
    std::shared_lock<std::shared_mutex> lock(m_documentMutex);
    return getRenderDataUnlocked(render_data);
}

bool MeshDocumentImpl::getRenderDataUnlocked(Render::RenderData& render_data) {
    MeshRenderInput input{m_nodes, m_elements};
    return MeshRenderBuilder::build(render_data, input);
}
// =============================================================================
// Factory
// =============================================================================
MeshDocumentImplSingletonFactory::tObjectSharedPtr
MeshDocumentImplSingletonFactory::instance() const {
    return MeshDocumentImpl::instance();
}
} // namespace OpenGeoLab::Mesh