/**
 * @file geometry_model.hpp
 * @brief Geometry model container for storing imported CAD data.
 *
 * Provides the central data storage for geometry imported from CAD files.
 * IO readers populate this structure, and App layer queries it for QML display.
 */
#pragma once

#include "geometry/geometry_types.hpp"

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
 * and rendering data (tessellated meshes).
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
     */
    bool isEmpty() const;

    /**
     * @brief Get summary statistics of the geometry.
     */
    std::string getSummary() const;

    /**
     * @brief Compute bounding box of all geometry.
     */
    BoundingBox computeBoundingBox() const;

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
};

using GeometryModelPtr = std::shared_ptr<GeometryModel>;

/**
 * @brief Singleton geometry store for the application.
 *
 * Provides thread-safe access to the current geometry model.
 * IO layer populates this, App layer reads from it.
 */
class GeometryStore {
public:
    static GeometryStore& instance();

    /**
     * @brief Set the current geometry model.
     */
    void setModel(GeometryModelPtr model);

    /**
     * @brief Get the current geometry model (may be nullptr).
     */
    GeometryModelPtr getModel() const;

    /**
     * @brief Clear the current model.
     */
    void clear();

    /**
     * @brief Check if a model is loaded.
     */
    bool hasModel() const;

private:
    GeometryStore() = default;

    mutable std::mutex m_mutex;
    GeometryModelPtr m_model;
};

} // namespace Geometry
} // namespace OpenGeoLab
