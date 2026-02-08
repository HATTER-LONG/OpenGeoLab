/**
 * @file mesh_document_impl.hpp
 * @brief Concrete implementation of MeshDocument
 */

#pragma once

#include "mesh/mesh_document.hpp"

#include <mutex>
#include <unordered_map>

namespace OpenGeoLab::Mesh {

/**
 * @brief Concrete mesh document storing nodes, elements, and geometry mappings
 *
 * Thread-safety: read operations acquire a shared lock; mutations acquire
 * an exclusive lock. Render data is cached and regenerated on demand.
 */
class MeshDocumentImpl : public MeshDocument,
                         public std::enable_shared_from_this<MeshDocumentImpl> {
public:
    MeshDocumentImpl();
    ~MeshDocumentImpl() override = default;

    // Node management
    bool addNode(const MeshNode& node) override;
    [[nodiscard]] const MeshNode* findNodeById(MeshElementId id) const override;
    [[nodiscard]] const MeshNode* findNodeByUID(MeshElementUID uid) const override;
    [[nodiscard]] size_t nodeCount() const override;
    [[nodiscard]] std::vector<MeshNode> allNodes() const override;

    // Element management
    bool addElement(const MeshElement& element) override;
    [[nodiscard]] const MeshElement* findElementById(MeshElementId id) const override;
    [[nodiscard]] const MeshElement* findElementByUID(MeshElementUID uid,
                                                      MeshEntityType type) const override;
    [[nodiscard]] size_t elementCount() const override;
    [[nodiscard]] std::vector<MeshElement> allElements() const override;
    [[nodiscard]] std::vector<MeshElement> elementsByType(MeshEntityType type) const override;

    // Geometry-Mesh mapping
    [[nodiscard]] std::vector<MeshElementKey>
    findElementsByGeometry(const Geometry::EntityKey& geo_key) const override;
    [[nodiscard]] std::vector<MeshElementKey>
    findNodesByGeometry(const Geometry::EntityKey& geo_key) const override;
    [[nodiscard]] Geometry::EntityKey geometryForElement(MeshElementUID elem_uid,
                                                         MeshEntityType elem_type) const override;
    [[nodiscard]] Geometry::EntityKey geometryForNode(MeshElementUID node_uid) const override;

    // Render
    [[nodiscard]] Render::DocumentRenderData getMeshRenderData() const override;

    // Document ops
    void clear() override;
    [[nodiscard]] bool isEmpty() const override;

private:
    /// Invalidate cached render data
    void invalidateRenderCache();

    /// Build render data from current mesh state
    [[nodiscard]] Render::DocumentRenderData buildRenderData() const;

    mutable std::mutex m_mutex;

    // ---- Node storage ----
    std::vector<MeshNode> m_nodes;
    std::unordered_map<MeshElementId, size_t> m_nodeById;
    std::unordered_map<MeshElementUID, size_t> m_nodeByUid;

    // ---- Element storage ----
    std::vector<MeshElement> m_elements;
    std::unordered_map<MeshElementId, size_t> m_elementById;

    struct TypeUidKey {
        MeshElementUID uid;
        MeshEntityType type;
        bool operator==(const TypeUidKey& o) const { return uid == o.uid && type == o.type; }
    };
    struct TypeUidKeyHash {
        size_t operator()(const TypeUidKey& k) const {
            size_t h = std::hash<MeshElementUID>{}(k.uid);
            h ^= std::hash<uint8_t>{}(static_cast<uint8_t>(k.type)) + 0x9e3779b97f4a7c15ULL +
                 (h << 6) + (h >> 2);
            return h;
        }
    };
    std::unordered_map<TypeUidKey, size_t, TypeUidKeyHash> m_elementByTypeUid;

    // ---- Geometry → Mesh mapping ----
    Geometry::EntityKeyMap<std::vector<MeshElementKey>> m_geoToElements;
    Geometry::EntityKeyMap<std::vector<MeshElementKey>> m_geoToNodes;

    // ---- Render data cache ----
    mutable Render::DocumentRenderData m_renderDataCache;
    mutable bool m_renderDataValid{false};
};

} // namespace OpenGeoLab::Mesh
