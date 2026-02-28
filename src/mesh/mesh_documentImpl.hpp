/**
 * @file mesh_documentImpl.hpp
 * @brief Concrete MeshDocument implementation — singleton that owns all
 *        mesh nodes/elements and provides thread-safe render data generation.
 */

#pragma once

#include "mesh/mesh_document.hpp"

namespace OpenGeoLab::Mesh {
/**
 * @brief Singleton MeshDocument implementation.
 *
 * Stores mesh nodes and elements in flat vectors. Render data is generated
 * on demand via MeshRenderBuilder and protected by a mutex for thread safety.
 */
class MeshDocumentImpl : public MeshDocument,
                         public std::enable_shared_from_this<MeshDocumentImpl> {

private:
    MeshDocumentImpl() = default;

public:
    /**
     * @brief Get the singleton instance.
     */
    static std::shared_ptr<MeshDocumentImpl> instance();
    ~MeshDocumentImpl() override = default;

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------

    bool addNode(MeshNode node) override;

    [[nodiscard]] MeshNode findNodeById(MeshNodeId node_id) const override;

    [[nodiscard]] size_t nodeCount() const override;

    // -------------------------------------------------------------------------
    // Element Management
    // -------------------------------------------------------------------------

    bool addElement(MeshElement element) override;

    [[nodiscard]] MeshElement findElementById(MeshElementId element_id) const override;

    [[nodiscard]] MeshElement findElementByRef(const MeshElementRef& ref) const override;

    [[nodiscard]] size_t elementCount() const override;

    // -------------------------------------------------------------------------
    // Edge Element Construction
    // -------------------------------------------------------------------------

    void buildEdgeElements() override;

    // -------------------------------------------------------------------------
    // Relation Queries (node ↔ line ↔ element)
    // -------------------------------------------------------------------------

    [[nodiscard]] std::vector<MeshElementRef> findLinesByNodeId(MeshNodeId node_id) const override;

    [[nodiscard]] std::vector<MeshElementRef>
    findElementsByNodeId(MeshNodeId node_id) const override;

    [[nodiscard]] std::vector<MeshElementRef>
    findElementsByLineRef(const MeshElementRef& line_ref) const override;

    [[nodiscard]] std::vector<MeshElementRef>
    findLinesByElementRef(const MeshElementRef& element_ref) const override;
    // -------------------------------------------------------------------------
    // Clear
    // -------------------------------------------------------------------------

    void clear() override;

    // -------------------------------------------------------------------------
    // Render Data
    // -------------------------------------------------------------------------

    [[nodiscard]] bool getRenderData(Render::RenderData& render_data) override;

    // -------------------------------------------------------------------------
    // Change Notification
    // -------------------------------------------------------------------------
    [[nodiscard]] Util::ScopedConnection
    subscribeToChanges(std::function<void()> callback) override;

    void notifyChanged() override;

private:
    std::vector<MeshNode> m_nodes;       ///< List of mesh nodes
    std::vector<MeshElement> m_elements; ///< List of mesh elements
    std::unordered_map<MeshElementRef, MeshElementId, MeshElementRefHash>
        m_refToId; ///< Fast lookup for elements by (uid, type)

    // ---- Relation maps (populated by buildEdgeElements) ----

    /// Node → Line element refs (which Line elements reference this node)
    std::unordered_map<MeshNodeId, std::vector<MeshElementRef>> m_nodeToLines;

    /// Node → non-Line element refs (which face/volume elements reference this node)
    std::unordered_map<MeshNodeId, std::vector<MeshElementRef>> m_nodeToElements;

    /// Line element ref → non-Line element refs sharing that edge
    MeshElementRefMap<std::vector<MeshElementRef>> m_lineToElements;

    /// Non-Line element ref → Line element refs forming the element's edges
    MeshElementRefMap<std::vector<MeshElementRef>> m_elementToLines;

    /// Sorted node pair → Line element ref for edge deduplication
    std::unordered_map<uint64_t, MeshElementRef> m_edgeKeyToLineRef;

    Util::Signal<> m_changeSignal; /// Change notification signal

    mutable std::mutex m_renderDataMutex;

    /// Create Line elements from edges of 2D/3D elements, populating m_edgeKeyToLineRef.
    void createLineElementsFromEdges();

    /// Build all node↔line↔element relation maps from current elements.
    void buildRelationMaps();
};

/**
 * @brief Singleton factory for MeshDocumentImpl
 */
class MeshDocumentImplSingletonFactory : public MeshDocumentSingletonFactory {
public:
    MeshDocumentImplSingletonFactory() = default;
    ~MeshDocumentImplSingletonFactory() override = default;

    tObjectSharedPtr instance() const override;
};
} // namespace OpenGeoLab::Mesh