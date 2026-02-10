/**
 * @file mesh_documentImpl.hpp
 * @brief Concrete implementation of MeshDocument
 *
 * Stores mesh nodes and elements using flat maps, provides O(1) lookup
 * by id and per-type lookup by (uid, type). Generates render data for
 * visualization of mesh wireframe and nodes.
 */

#pragma once

#include "mesh/mesh_document.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace OpenGeoLab::Mesh {

/**
 * @brief Concrete mesh document implementation with hash-map storage
 */
class MeshDocumentImpl : public MeshDocument,
                         public std::enable_shared_from_this<MeshDocumentImpl> {
public:
    /**
     * @brief Get the singleton instance.
     */
    static std::shared_ptr<MeshDocumentImpl> instance();

    MeshDocumentImpl() = default;
    ~MeshDocumentImpl() override = default;

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------

    bool addNode(const MeshNode& node) override;

    [[nodiscard]] const MeshNode* findNodeById(MeshNodeId node_id) const override;

    [[nodiscard]] size_t nodeCount() const override;

    // -------------------------------------------------------------------------
    // Element Management
    // -------------------------------------------------------------------------

    bool addElement(MeshElement element) override;

    [[nodiscard]] const MeshElement* findElementById(MeshElementId element_id) const override;

    [[nodiscard]] const MeshElement* findElementByRef(const MeshElementRef& ref) const override;

    [[nodiscard]] size_t elementCount() const override;

    [[nodiscard]] std::vector<const MeshElement*>
    elementsByType(MeshElementType type) const override;

    // -------------------------------------------------------------------------
    // Clear
    // -------------------------------------------------------------------------

    void clear() override;

    // -------------------------------------------------------------------------
    // Render Data
    // -------------------------------------------------------------------------

    [[nodiscard]] Render::DocumentRenderData getRenderData() const override;

private:
    /**
     * @brief Generate edge wireframe render meshes for 2D elements.
     */
    void generateElementEdgeRenderData(Render::DocumentRenderData& data) const;

    /**
     * @brief Generate node point render meshes.
     */
    void generateNodeRenderData(Render::DocumentRenderData& data) const;

private:
    /// Node storage: nodeId -> MeshNode
    std::unordered_map<MeshNodeId, MeshNode> m_nodes;

    /// Element storage: elementId -> MeshElement
    std::unordered_map<MeshElementId, MeshElement> m_elements;

    /// Per-type element index: (uid, type) -> elementId
    std::unordered_map<MeshElementRef, MeshElementId, MeshElementRefHash> m_refToId;
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
