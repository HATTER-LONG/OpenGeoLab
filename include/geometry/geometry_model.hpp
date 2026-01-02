/**
 * @file geometry_model.hpp
 * @brief Geometry model container for storing imported CAD data.
 *
 * Provides the central data storage for geometry imported from CAD files.
 * IO readers populate this structure, and App layer queries it for QML display.
 * Includes a global signal mechanism to notify listeners when geometry data changes.
 */
#pragma once

#include "geometry/geometry_types.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief Complete geometry data structure from model import.
 *
 * Contains hierarchical topology (parts -> solids -> faces -> edges -> vertices)
 * and rendering data (tessellated meshes). Supports OCC-based model import and
 * programmatic geometry creation (e.g., creating boxes).
 */
class GeometryModel {
public:
    GeometryModel() = default;
    ~GeometryModel() = default;

    // Non-copyable, movable
    GeometryModel(const GeometryModel&) = delete;
    GeometryModel& operator=(const GeometryModel&) = delete;
    GeometryModel(GeometryModel&&) = default;
    GeometryModel& operator=(GeometryModel&&) = default;

    /**
     * @brief Clear all geometry data.
     */
    void clear();

    /**
     * @brief Check if geometry data is empty.
     * @return True if no geometry entities exist.
     */
    bool isEmpty() const;

    /**
     * @brief Get summary statistics of the geometry.
     * @return Human-readable summary string.
     */
    std::string getSummary() const;

    /**
     * @brief Compute bounding box of all geometry.
     * @return Bounding box encompassing all vertices.
     */
    BoundingBox computeBoundingBox() const;

    /**
     * @brief Generate next unique ID for geometry entities.
     * @return Unique ID for a new entity.
     */
    uint32_t generateNextId();

    // Part accessors
    const std::vector<Part>& getParts() const { return m_parts; }
    void addPart(Part part);
    const Part* getPartById(uint32_t id) const;

    // Solid accessors
    const std::vector<Solid>& getSolids() const { return m_solids; }
    void addSolid(Solid solid);
    const Solid* getSolidById(uint32_t id) const;

    // Face accessors
    const std::vector<Face>& getFaces() const { return m_faces; }
    void addFace(Face face);
    const Face* getFaceById(uint32_t id) const;

    // Edge accessors
    const std::vector<Edge>& getEdges() const { return m_edges; }
    void addEdge(Edge edge);
    const Edge* getEdgeById(uint32_t id) const;

    // Vertex accessors
    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    void addVertex(Vertex vertex);
    const Vertex* getVertexById(uint32_t id) const;

    // Count accessors
    size_t partCount() const { return m_parts.size(); }
    size_t solidCount() const { return m_solids.size(); }
    size_t faceCount() const { return m_faces.size(); }
    size_t edgeCount() const { return m_edges.size(); }
    size_t vertexCount() const { return m_vertices.size(); }

    /**
     * @brief Source file path of the imported model.
     */
    std::string m_sourcePath;

private:
    std::vector<Part> m_parts;
    std::vector<Solid> m_solids;
    std::vector<Face> m_faces;
    std::vector<Edge> m_edges;
    std::vector<Vertex> m_vertices;

    uint32_t m_nextId = 1; ///< Counter for generating unique IDs.
};

using GeometryModelPtr = std::shared_ptr<GeometryModel>;

/**
 * @brief Callback type for geometry change notifications.
 *
 * Listeners receive this callback when geometry data is modified.
 */
using GeometryChangedCallback = std::function<void()>;

/**
 * @brief Singleton geometry store for the application.
 *
 * Provides thread-safe access to the current geometry model.
 * IO layer populates this, App layer reads from it.
 * Supports callback registration for change notifications.
 */
class GeometryStore {
public:
    /**
     * @brief Get singleton instance.
     * @return Reference to the global GeometryStore.
     */
    static GeometryStore& instance();

    /**
     * @brief Set the current geometry model and notify listeners.
     * @param model New geometry model (may be nullptr to clear).
     */
    void setModel(GeometryModelPtr model);

    /**
     * @brief Get the current geometry model.
     * @return Shared pointer to current model (may be nullptr).
     */
    GeometryModelPtr getModel() const;

    /**
     * @brief Clear the current model and notify listeners.
     */
    void clear();

    /**
     * @brief Check if a non-empty model is loaded.
     * @return True if model exists and contains geometry.
     */
    bool hasModel() const;

    /**
     * @brief Register a callback for geometry change notifications.
     * @param callback Function to call when geometry changes.
     * @return Unique ID for the registered callback (for unregistration).
     * @note Callbacks are invoked synchronously after model changes.
     */
    size_t registerChangeCallback(GeometryChangedCallback callback);

    /**
     * @brief Unregister a previously registered callback.
     * @param callback_id ID returned from registerChangeCallback().
     */
    void unregisterChangeCallback(size_t callback_id);

    /**
     * @brief Manually notify all listeners of geometry changes.
     * @note Called automatically by setModel() and clear().
     *       Use this when modifying the model directly.
     */
    void notifyGeometryChanged();

private:
    GeometryStore() = default;

    mutable std::mutex m_mutex;
    GeometryModelPtr m_model;

    std::mutex m_callbackMutex;
    std::vector<std::pair<size_t, GeometryChangedCallback>> m_callbacks;
    size_t m_nextCallbackId = 1; ///< Counter for callback IDs.
};

} // namespace Geometry
} // namespace OpenGeoLab
