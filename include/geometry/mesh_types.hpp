/**
 * @file mesh_types.hpp
 * @brief Mesh data types for mesh generation and quality analysis
 *
 * Defines mesh data structures including:
 * - Mesh nodes and elements
 * - Mesh quality metrics
 * - Mesh regions for different materials
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace Mesh {

/**
 * @brief Mesh node with position and properties
 */
struct MeshNode {
    uint32_t m_id = 0;               ///< Unique node identifier
    Geometry::Point3D m_position;    ///< 3D position
    bool m_isBoundary = false;       ///< Is node on boundary
    uint32_t m_geometryVertexId = 0; ///< Associated geometry vertex ID (0 if none)
};

/**
 * @brief Element types for mesh
 */
enum class ElementType {
    Triangle,    ///< 2D triangle (3 nodes)
    Quad,        ///< 2D quadrilateral (4 nodes)
    Tetrahedron, ///< 3D tetrahedron (4 nodes)
    Hexahedron,  ///< 3D hexahedron (8 nodes)
    Wedge,       ///< 3D wedge/prism (6 nodes)
    Pyramid      ///< 3D pyramid (5 nodes)
};

/**
 * @brief Mesh element (cell)
 */
struct MeshElement {
    uint32_t m_id = 0;                          ///< Unique element identifier
    ElementType m_type = ElementType::Triangle; ///< Element type
    std::vector<uint32_t> m_nodeIds;            ///< Node IDs (connectivity)
    uint32_t m_regionId = 0;                    ///< Region/material ID
    uint32_t m_geometryFaceId = 0;              ///< Associated geometry face ID (for surface mesh)
};

/**
 * @brief Mesh quality metrics for an element
 */
struct ElementQuality {
    uint32_t m_elementId = 0;   ///< Element ID
    double m_aspectRatio = 1.0; ///< Aspect ratio (1.0 = ideal)
    double m_skewness = 0.0;    ///< Skewness (0.0 = ideal)
    double m_minAngle = 60.0;   ///< Minimum angle in degrees
    double m_maxAngle = 60.0;   ///< Maximum angle in degrees
    double m_jacobian = 1.0;    ///< Jacobian determinant (positive = valid)
    double m_quality = 1.0;     ///< Overall quality score (0-1)
    bool m_isValid = true;      ///< Is element valid (not inverted)
};

/**
 * @brief Quality thresholds for mesh validation
 */
struct QualityThresholds {
    double maxAspectRatio = 10.0; ///< Maximum acceptable aspect ratio
    double maxSkewness = 0.9;     ///< Maximum acceptable skewness
    double minAngle = 10.0;       ///< Minimum acceptable angle
    double maxAngle = 160.0;      ///< Maximum acceptable angle
    double minJacobian = 0.1;     ///< Minimum acceptable Jacobian
    double minQuality = 0.1;      ///< Minimum acceptable quality score
};

/**
 * @brief Mesh quality summary
 */
struct MeshQualitySummary {
    size_t totalElements = 0;       ///< Total number of elements
    size_t validElements = 0;       ///< Number of valid elements
    size_t invalidElements = 0;     ///< Number of invalid elements
    size_t poorQualityElements = 0; ///< Elements below quality threshold
    double minQuality = 0.0;        ///< Minimum quality score
    double maxQuality = 0.0;        ///< Maximum quality score
    double avgQuality = 0.0;        ///< Average quality score
    double minAspectRatio = 0.0;    ///< Minimum aspect ratio
    double maxAspectRatio = 0.0;    ///< Maximum aspect ratio
    double avgAspectRatio = 0.0;    ///< Average aspect ratio
};

/**
 * @brief Mesh region (material zone)
 */
struct MeshRegion {
    uint32_t m_id = 0;                  ///< Unique region identifier
    std::string m_name;                 ///< Region name
    uint32_t m_materialId = 0;          ///< Associated material ID
    std::vector<uint32_t> m_elementIds; ///< Elements in this region
};

/**
 * @brief Complete mesh data structure
 */
struct MeshData {
    std::vector<MeshNode> m_nodes;       ///< All mesh nodes
    std::vector<MeshElement> m_elements; ///< All mesh elements
    std::vector<MeshRegion> m_regions;   ///< Mesh regions

    // Cached quality data
    std::vector<ElementQuality> m_elementQualities;
    MeshQualitySummary m_qualitySummary;

    /**
     * @brief Check if mesh is empty
     * @return True if no nodes or elements
     */
    bool isEmpty() const { return m_nodes.empty() || m_elements.empty(); }

    /**
     * @brief Clear all mesh data
     */
    void clear();

    /**
     * @brief Get node by ID
     * @param id Node ID
     * @return Pointer to node or nullptr
     */
    const MeshNode* getNodeById(uint32_t id) const;

    /**
     * @brief Get element by ID
     * @param id Element ID
     * @return Pointer to element or nullptr
     */
    const MeshElement* getElementById(uint32_t id) const;

    /**
     * @brief Compute quality metrics for all elements
     */
    void computeQuality();

    /**
     * @brief Get elements with poor quality
     * @param thresholds Quality thresholds
     * @return Vector of poor quality element IDs
     */
    std::vector<uint32_t> getPoorQualityElements(const QualityThresholds& thresholds) const;

    /**
     * @brief Get mesh summary string
     * @return Human-readable summary
     */
    std::string getSummary() const;
};

using Point3D = Geometry::Point3D;

} // namespace Mesh
} // namespace OpenGeoLab
