/**
 * @file render_data.hpp
 * @brief Render data structures for OpenGL rendering
 *
 * Provides data structures for converting geometry model to renderable format.
 */

#pragma once

#include <QVector3D>

#include <cstdint>
#include <vector>

namespace OpenGeoLab {
namespace Render {

/**
 * @brief Vertex data for OpenGL rendering
 */
struct RenderVertex {
    QVector3D position;
    QVector3D normal;
    QVector3D color;

    RenderVertex() = default;
    RenderVertex(const QVector3D& pos, const QVector3D& norm, const QVector3D& col)
        : position(pos), normal(norm), color(col) {}
};

/**
 * @brief Geometry data for rendering
 *
 * Contains vertex and index data ready for OpenGL upload.
 */
struct RenderGeometry {
    std::vector<RenderVertex> vertices;
    std::vector<uint32_t> indices;

    /**
     * @brief Check if geometry is empty
     * @return True if no vertices exist
     */
    bool isEmpty() const { return vertices.empty(); }

    /**
     * @brief Clear all data
     */
    void clear();

    /**
     * @brief Get number of triangles
     * @return Triangle count
     */
    size_t triangleCount() const { return indices.size() / 3; }

    /**
     * @brief Get bounding box minimum
     * @return Minimum corner
     */
    QVector3D boundingBoxMin() const;

    /**
     * @brief Get bounding box maximum
     * @return Maximum corner
     */
    QVector3D boundingBoxMax() const;

    /**
     * @brief Get center of bounding box
     * @return Center point
     */
    QVector3D center() const;
};

} // namespace Render
} // namespace OpenGeoLab
