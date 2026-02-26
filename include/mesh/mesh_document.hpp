/**
 * @file mesh_document.hpp
 * @brief Abstract mesh document interface for node and element management
 *
 * MeshDocument is the primary container for FEM mesh data. It stores
 * mesh nodes and elements, supports queries by various keys, and
 * provides render data for visualization.
 */

#pragma once

#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"
#include "render/render_data.hpp"
#include "util/signal.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>

namespace OpenGeoLab::Mesh {

class MeshDocument;
using MeshDocumentPtr = std::shared_ptr<MeshDocument>;

/**
 * @brief Abstract mesh document interface
 *
 * A mesh document stores FEM mesh nodes and elements. Implementations
 * provide storage, queries by MeshElementKey/MeshElementRef, render data
 * generation, and association with source geometry entities.
 */
class MeshDocument : public Kangaroo::Util::NonCopyable {
public:
    MeshDocument() = default;
    virtual ~MeshDocument() = default;

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a node to the document.
     * @param node Node to add (moved).
     * @return true if added; false on duplicate id.
     */
    virtual bool addNode(MeshNode node) = 0;

    /**
     * @brief Find a node by id.
     * @param node_id Node id to find.
     * @return Copy of the node, or throws if not found.
     */
    [[nodiscard]] virtual MeshNode findNodeById(MeshNodeId node_id) const = 0;

    /**
     * @brief Get total node count.
     */
    [[nodiscard]] virtual size_t nodeCount() const = 0;

    // -------------------------------------------------------------------------
    // Element Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add an element to the document.
     * @param element Element to add (moved).
     * @return true if added; false on duplicate id.
     */
    virtual bool addElement(MeshElement element) = 0;

    /**
     * @brief Find an element by global id.
     * @param element_id Global element id.
     * @return Copy of the element, or throws if not found.
     */
    [[nodiscard]] virtual MeshElement findElementById(MeshElementId element_id) const = 0;

    /**
     * @brief Find an element by (uid, type) reference.
     * @param ref Element reference with uid + type.
     * @return Copy of the element, or throws if not found.
     */
    [[nodiscard]] virtual MeshElement findElementByRef(const MeshElementRef& ref) const = 0;

    /**
     * @brief Get total element count.
     * @return Total number of elements in the document.
     */
    [[nodiscard]] virtual size_t elementCount() const = 0;

    // -------------------------------------------------------------------------
    // Edge Element Construction
    // -------------------------------------------------------------------------

    /**
     * @brief Build Line elements from edges of 2D/3D elements and populate relation maps.
     *
     * Scans all existing non-Line elements, extracts unique edges (node pairs),
     * creates MeshElement(Line) for each unique edge not already present,
     * and builds node↔line↔element lookup tables.
     * Must be called after bulk element addition (e.g., after Gmsh import).
     */
    virtual void buildEdgeElements() = 0;

    // -------------------------------------------------------------------------
    // Relation Queries (node ↔ line ↔ element)
    // -------------------------------------------------------------------------

    /**
     * @brief Find all Line elements containing a given node.
     * @param node_id Node to query.
     * @return Refs of Line elements, empty if none or node not found.
     */
    [[nodiscard]] virtual std::vector<MeshElementRef>
    findLinesByNodeId(MeshNodeId node_id) const = 0;

    /**
     * @brief Find all non-Line elements containing a given node.
     * @param node_id Node to query.
     * @return Refs of face/volume elements, empty if none or node not found.
     */
    [[nodiscard]] virtual std::vector<MeshElementRef>
    findElementsByNodeId(MeshNodeId node_id) const = 0;

    /**
     * @brief Find all non-Line elements that share a given edge (Line element).
     * @param line_ref Reference to the Line element.
     * @return Refs of elements sharing this edge, empty if none.
     */
    [[nodiscard]] virtual std::vector<MeshElementRef>
    findElementsByLineRef(const MeshElementRef& line_ref) const = 0;

    /**
     * @brief Find all Line elements that are edges of a given non-Line element.
     * @param element_ref Reference to the face/volume element.
     * @return Refs of Line elements forming the element's edges, empty if none.
     */
    [[nodiscard]] virtual std::vector<MeshElementRef>
    findLinesByElementRef(const MeshElementRef& element_ref) const = 0;

    // -------------------------------------------------------------------------
    // Clear
    // -------------------------------------------------------------------------

    /**
     * @brief Clear all nodes, elements, and relation maps.
     */
    virtual void clear() = 0;

    // -------------------------------------------------------------------------
    // Render Data
    // -------------------------------------------------------------------------

    /**
     * @brief Generate render data from current mesh state.
     * @param render_data Output render data to populate.
     * @return true on success.
     */
    [[nodiscard]] virtual bool getRenderData(Render::RenderData& render_data) = 0;

    // -------------------------------------------------------------------------
    // Change Notification
    // -------------------------------------------------------------------------

    /**
     * @brief Subscribe to mesh data changes.
     * @param callback Callback executed when mesh data changes.
     * @return Scoped connection that disconnects on destruction.
     */
    [[nodiscard]] virtual Util::ScopedConnection
    subscribeToChanges(std::function<void()> callback) = 0;

    /**
     * @brief Notify that mesh data has changed.
     *
     * Call after bulk operations (e.g., mesh generation) to trigger
     * render updates. Individual addNode/addElement calls should NOT
     * trigger this; callers should call notifyChanged() explicitly
     * after completing bulk operations.
     */
    virtual void notifyChanged() = 0;
};

/**
 * @brief Singleton factory interface for MeshDocument
 */
class MeshDocumentSingletonFactory
    : public Kangaroo::Util::FactoryTraits<MeshDocumentSingletonFactory, MeshDocument> {
public:
    MeshDocumentSingletonFactory() = default;
    virtual ~MeshDocumentSingletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace OpenGeoLab::Mesh

#define MeshDocumentInstance                                                                       \
    g_ComponentFactory.getInstanceObject<Mesh::MeshDocumentSingletonFactory>()
