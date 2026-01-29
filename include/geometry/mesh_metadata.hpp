/**
 * @file mesh_metadata.hpp
 * @brief Geometry metadata for mesh generation
 *
 * MeshMetadata provides geometric properties extracted from OCC shapes
 * that are useful for mesh generation algorithms. This includes curvature
 * information, surface properties, and size hints.
 */

#pragma once

#include "geometry_types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Surface type classification for mesh sizing
 */
enum class SurfaceType : uint8_t {
    Unknown = 0,     ///< Unknown surface type
    Planar = 1,      ///< Flat plane
    Cylindrical = 2, ///< Cylinder or cone
    Spherical = 3,   ///< Sphere
    Toroidal = 4,    ///< Torus
    BSpline = 5,     ///< B-spline surface
    Bezier = 6,      ///< Bezier surface
    Revolution = 7,  ///< Surface of revolution
    Extrusion = 8,   ///< Extruded surface
    Offset = 9       ///< Offset surface
};

/**
 * @brief Curve type classification
 */
enum class CurveType : uint8_t {
    Unknown = 0,   ///< Unknown curve type
    Line = 1,      ///< Straight line
    Circle = 2,    ///< Full or partial circle
    Ellipse = 3,   ///< Ellipse
    Parabola = 4,  ///< Parabola
    Hyperbola = 5, ///< Hyperbola
    BSpline = 6,   ///< B-spline curve
    Bezier = 7     ///< Bezier curve
};

/**
 * @brief Curvature information at a surface point
 */
struct SurfaceCurvature {
    double m_minCurvature{0.0};      ///< Minimum principal curvature (1/radius)
    double m_maxCurvature{0.0};      ///< Maximum principal curvature (1/radius)
    double m_gaussianCurvature{0.0}; ///< Gaussian curvature (k1 * k2)
    double m_meanCurvature{0.0};     ///< Mean curvature ((k1 + k2) / 2)

    Vector3D m_minDirection; ///< Direction of minimum curvature
    Vector3D m_maxDirection; ///< Direction of maximum curvature

    /// Default constructor
    SurfaceCurvature() = default;

    /**
     * @brief Check if the surface is locally flat
     * @param tolerance Curvature tolerance
     * @return true if both principal curvatures are near zero
     */
    [[nodiscard]] bool isFlat(double tolerance = 1e-6) const {
        return std::fabs(m_minCurvature) < tolerance && std::fabs(m_maxCurvature) < tolerance;
    }

    /**
     * @brief Get characteristic length scale based on curvature
     * @return Suggested mesh size based on curvature
     *
     * Returns infinity for flat regions, radius of curvature otherwise.
     */
    [[nodiscard]] double characteristicLength() const;
};

/**
 * @brief Metadata for a single edge entity
 */
struct EdgeMetadata {
    EntityId m_entityId{INVALID_ENTITY_ID};    ///< Source edge entity ID
    CurveType m_curveType{CurveType::Unknown}; ///< Underlying curve type
    double m_length{0.0};                      ///< Edge length
    double m_maxCurvature{0.0};                ///< Maximum curvature along edge
    bool m_isDegenerate{false};                ///< True if edge is degenerate (zero length)
    bool m_isSeam{false};                      ///< True if edge is a surface seam

    Point3D m_startPoint; ///< Start vertex position
    Point3D m_endPoint;   ///< End vertex position

    /// Default constructor
    EdgeMetadata() = default;

    /**
     * @brief Suggest mesh size for this edge
     * @param base_size Base mesh size
     * @param curvature_factor Factor to scale size by curvature
     * @return Suggested number of segments for this edge
     */
    [[nodiscard]] size_t suggestSegmentCount(double base_size, double curvature_factor = 0.2) const;
};

/**
 * @brief Metadata for a single face entity
 */
struct FaceMetadata {
    EntityId m_entityId{INVALID_ENTITY_ID};          ///< Source face entity ID
    SurfaceType m_surfaceType{SurfaceType::Unknown}; ///< Underlying surface type
    double m_area{0.0};                              ///< Face area
    double m_minCurvature{0.0};                      ///< Minimum curvature on face
    double m_maxCurvature{0.0};                      ///< Maximum curvature on face
    bool m_isForward{true};                          ///< Face orientation (true = outward normal)

    BoundingBox3D m_boundingBox; ///< Face bounding box

    /// UV parameter bounds
    double m_uMin{0.0}, m_uMax{1.0};
    double m_vMin{0.0}, m_vMax{1.0};

    std::vector<EntityId> m_boundaryEdges; ///< IDs of boundary edges
    std::vector<EntityId> m_holeEdges;     ///< IDs of hole boundary edges

    /// Default constructor
    FaceMetadata() = default;

    /**
     * @brief Suggest mesh size for this face
     * @param base_size Base mesh size
     * @param curvature_factor Factor to scale size by curvature
     * @return Suggested characteristic mesh element size
     */
    [[nodiscard]] double suggestMeshSize(double base_size, double curvature_factor = 0.2) const;

    /**
     * @brief Sample curvature at a UV point
     * @param u U parameter
     * @param v V parameter
     * @return Curvature information at (u, v)
     *
     * @note Requires the source face entity to compute actual values.
     */
    [[nodiscard]] SurfaceCurvature curvatureAt(double u, double v) const;
};

/**
 * @brief Metadata for a single solid entity
 */
struct SolidMetadata {
    EntityId m_entityId{INVALID_ENTITY_ID}; ///< Source solid entity ID
    double m_volume{0.0};                   ///< Solid volume
    double m_surfaceArea{0.0};              ///< Total surface area
    Point3D m_centerOfMass;                 ///< Center of mass
    BoundingBox3D m_boundingBox;            ///< Solid bounding box

    size_t m_faceCount{0};   ///< Number of faces
    size_t m_edgeCount{0};   ///< Number of edges
    size_t m_vertexCount{0}; ///< Number of vertices

    std::vector<EntityId> m_faceIds; ///< IDs of constituent faces

    /// Default constructor
    SolidMetadata() = default;

    /**
     * @brief Get characteristic length of the solid
     * @return Cube root of volume, or bounding box diagonal
     */
    [[nodiscard]] double characteristicLength() const;
};

/**
 * @brief Complete mesh metadata for a Part
 *
 * Aggregates all geometric properties needed for mesh generation.
 */
struct PartMeshMetadata {
    EntityId m_partEntityId{INVALID_ENTITY_ID}; ///< Source part entity ID
    std::string m_partName;                     ///< Part display name

    BoundingBox3D m_boundingBox;        ///< Part bounding box
    double m_characteristicLength{0.0}; ///< Overall characteristic size

    std::vector<SolidMetadata> m_solids; ///< Solid metadata
    std::vector<FaceMetadata> m_faces;   ///< Face metadata
    std::vector<EdgeMetadata> m_edges;   ///< Edge metadata

    /// Default constructor
    PartMeshMetadata() = default;

    /**
     * @brief Get suggested global mesh size
     * @param elements_per_characteristic Number of elements per characteristic length
     * @return Suggested global mesh element size
     */
    [[nodiscard]] double suggestGlobalMeshSize(double elements_per_characteristic = 10.0) const;

    /**
     * @brief Get all face IDs
     * @return Vector of face entity IDs
     */
    [[nodiscard]] std::vector<EntityId> getAllFaceIds() const;

    /**
     * @brief Get all edge IDs
     * @return Vector of edge entity IDs
     */
    [[nodiscard]] std::vector<EntityId> getAllEdgeIds() const;
};

using PartMeshMetadataPtr = std::shared_ptr<PartMeshMetadata>;

/**
 * @brief Complete mesh metadata for the entire document
 */
struct DocumentMeshMetadata {
    std::vector<PartMeshMetadataPtr> m_parts; ///< Metadata for each part
    BoundingBox3D m_sceneBoundingBox;         ///< Combined bounding box

    /// Default constructor
    DocumentMeshMetadata() = default;

    /// Get total part count
    [[nodiscard]] size_t partCount() const { return m_parts.size(); }

    /// Recompute scene bounding box from all parts
    void updateSceneBoundingBox();

    /**
     * @brief Get suggested global mesh size for the scene
     * @param elements_per_characteristic Number of elements per characteristic length
     * @return Suggested global mesh element size
     */
    [[nodiscard]] double suggestGlobalMeshSize(double elements_per_characteristic = 10.0) const;
};

using DocumentMeshMetadataPtr = std::shared_ptr<DocumentMeshMetadata>;

} // namespace OpenGeoLab::Geometry
