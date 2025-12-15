/**
 * @file geometry.hpp
 * @brief Geometry data structures for 3D rendering
 *
 * Defines abstract and concrete geometry data classes.
 * Separates vertex data from rendering logic for better modularity.
 * Each vertex contains: position (3 floats), normal (3 floats), color (3 floats).
 *
 * Note: Basic geometric primitives (Box, Cylinder, Sphere, etc.) should be created
 * using the GeometryCreator class which uses Open CASCADE Technology.
 */

#pragma once

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief Abstract base class for geometry data
 *
 * This abstract class defines the interface for geometry data,
 * allowing different geometric shapes to provide their vertex
 * and index data in a uniform way.
 */
struct GeometryData {
    virtual ~GeometryData() = default;

    /**
     * @brief Get vertex data pointer
     * @return Pointer to vertex array (format: pos(3) + normal(3) + color(3) per vertex)
     */
    virtual const float* vertices() const = 0;

    /**
     * @brief Get number of vertices
     * @return Total vertex count
     */
    virtual int vertexCount() const = 0;

    /**
     * @brief Get index data pointer (optional)
     * @return Pointer to index array, or nullptr if not using indexed drawing
     */
    virtual const unsigned int* indices() const { return nullptr; }

    /**
     * @brief Get number of indices (optional)
     * @return Total index count, or 0 if not using indexed drawing
     */
    virtual int indexCount() const { return 0; }

    /**
     * @brief Get bounding box of the geometry
     * @param min_point Output parameter for minimum corner (x, y, z)
     * @param max_point Output parameter for maximum corner (x, y, z)
     * @return true if bounding box is valid, false otherwise
     */
    virtual bool getBoundingBox(float min_point[3], float max_point[3]) const {
        if(vertexCount() == 0) {
            return false;
        }

        const float* verts = vertices();
        min_point[0] = min_point[1] = min_point[2] = std::numeric_limits<float>::max();
        max_point[0] = max_point[1] = max_point[2] = std::numeric_limits<float>::lowest();

        for(int i = 0; i < vertexCount(); ++i) {
            int idx = i * 9; // 9 floats per vertex (pos, normal, color)
            for(int j = 0; j < 3; ++j) {
                min_point[j] = std::min(min_point[j], verts[idx + j]);
                max_point[j] = std::max(max_point[j], verts[idx + j]);
            }
        }
        return true;
    }
};

/**
 * @brief Mesh geometry data for triangulated models
 *
 * Provides vertex data for meshes loaded from external files (BREP, STEP, etc.)
 * or created via Open CASCADE primitives.
 * Each vertex contains: position (3 floats), normal (3 floats), color (3 floats)
 * Total: 9 floats per vertex
 */
class MeshData : public GeometryData {
public:
    MeshData() = default;

    struct PartInfo {
        std::string m_name;
        int m_solidIndex = -1;
        int m_faceCount = 0;
        int m_edgeCount = 0;
    };

    /**
     * @brief Set vertex data (moves data to avoid copying)
     * @param vertex_data Vector containing position, normal, and color data (9 floats per vertex)
     */
    void setVertexData(std::vector<float>&& vertex_data) { m_vertexData = std::move(vertex_data); }

    /**
     * @brief Set index data (moves data to avoid copying)
     * @param index_data Vector containing triangle indices
     */
    void setIndexData(std::vector<unsigned int>&& index_data) {
        m_indexData = std::move(index_data);
    }

    void setParts(std::vector<PartInfo>&& parts) { m_parts = std::move(parts); }

    const std::vector<PartInfo>& parts() const { return m_parts; }

    const float* vertices() const override { return m_vertexData.data(); }

    int vertexCount() const override {
        return static_cast<int>(m_vertexData.size() / 9); // 9 floats per vertex
    }

    const unsigned int* indices() const override {
        return m_indexData.empty() ? nullptr : m_indexData.data();
    }

    int indexCount() const override { return static_cast<int>(m_indexData.size()); }

private:
    std::vector<float> m_vertexData;       // position(3) + normal(3) + color(3)
    std::vector<unsigned int> m_indexData; // Triangle indices
    std::vector<PartInfo> m_parts;
};

} // namespace Geometry
} // namespace OpenGeoLab
