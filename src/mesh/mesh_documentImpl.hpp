#pragma once

#include "mesh/mesh_document.hpp"

namespace OpenGeoLab::Mesh {
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

    Util::Signal<> m_changeSignal; /// Change notification signal
    // /// Cached render data
    // mutable Render::RenderData m_cachedRenderData;
    // mutable bool m_renderDataValid{false};
    mutable std::mutex m_renderDataMutex;
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