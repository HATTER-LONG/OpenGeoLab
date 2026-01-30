/**
 * @file mesh_metadata.hpp
 * @brief Geometry metadata structures for mesh generation
 *
 * Provides metadata extraction from OCC geometry entities for mesh generation
 * tools. Contains surface type information, curvature data, and sizing hints
 * for adaptive meshing algorithms.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <memory>

namespace OpenGeoLab::Geometry {

class FaceEntity;
class EdgeEntity;
using FaceEntityPtr = std::shared_ptr<FaceEntity>;
using EdgeEntityPtr = std::shared_ptr<EdgeEntity>;

/**
 * @brief Surface type classification for mesh generation
 */
enum class SurfaceType : uint8_t {
    Unknown = 0,             ///< Unknown surface type
    Plane = 1,               ///< Planar surface
    Cylinder = 2,            ///< Cylindrical surface
    Cone = 3,                ///< Conical surface
    Sphere = 4,              ///< Spherical surface
    Torus = 5,               ///< Toroidal surface
    BezierSurface = 6,       ///< Bezier surface
    BSplineSurface = 7,      ///< B-spline surface
    SurfaceOfRevolution = 8, ///< Surface of revolution
    SurfaceOfExtrusion = 9,  ///< Surface of extrusion
    OffsetSurface = 10       ///< Offset surface
};

/**
 * @brief Curve type classification for mesh generation
 */
enum class CurveType : uint8_t {
    Unknown = 0,     ///< Unknown curve type
    Line = 1,        ///< Straight line
    Circle = 2,      ///< Circular arc
    Ellipse = 3,     ///< Elliptical arc
    Hyperbola = 4,   ///< Hyperbolic arc
    Parabola = 5,    ///< Parabolic arc
    BezierCurve = 6, ///< Bezier curve
    BSplineCurve = 7 ///< B-spline curve
};

/**
 * @brief Curvature information at a point
 */
struct CurvatureInfo {
    double m_minCurvature{0.0};      ///< Minimum principal curvature
    double m_maxCurvature{0.0};      ///< Maximum principal curvature
    double m_gaussianCurvature{0.0}; ///< Gaussian curvature (product of principal curvatures)
    double m_meanCurvature{0.0};     ///< Mean curvature (average of principal curvatures)
    Vector3D m_minDirection;         ///< Direction of minimum curvature
    Vector3D m_maxDirection;         ///< Direction of maximum curvature
};

/**
 * @brief Metadata for a face entity used in mesh generation
 *
 * Contains geometric information needed by mesh generation algorithms
 * to determine appropriate element sizes and distribution.
 */
struct FaceMeshMetadata {
    EntityId m_entityId{INVALID_ENTITY_ID};          ///< Source face entity ID
    SurfaceType m_surfaceType{SurfaceType::Unknown}; ///< Surface type classification

    double m_area{0.0};      ///< Face area
    double m_perimeter{0.0}; ///< Total boundary length

    // UV parameter bounds
    double m_uMin{0.0}; ///< Minimum U parameter
    double m_uMax{1.0}; ///< Maximum U parameter
    double m_vMin{0.0}; ///< Minimum V parameter
    double m_vMax{1.0}; ///< Maximum V parameter

    // Curvature statistics
    double m_minCurvature{0.0}; ///< Minimum curvature on the surface
    double m_maxCurvature{0.0}; ///< Maximum curvature on the surface
    double m_avgCurvature{0.0}; ///< Average curvature on the surface

    // Sizing hints for meshing
    double m_suggestedElementSize{1.0}; ///< Suggested element size based on curvature

    size_t m_boundaryEdgeCount{0}; ///< Number of boundary edges
    size_t m_holeCount{0};         ///< Number of holes in the face

    bool m_isReversed{false}; ///< Whether face orientation is reversed
};

/**
 * @brief Metadata for an edge entity used in mesh generation
 */
struct EdgeMeshMetadata {
    EntityId m_entityId{INVALID_ENTITY_ID};    ///< Source edge entity ID
    CurveType m_curveType{CurveType::Unknown}; ///< Curve type classification

    double m_length{0.0};     ///< Edge length
    double m_firstParam{0.0}; ///< Start parameter
    double m_lastParam{1.0};  ///< End parameter

    // Curvature statistics
    double m_minCurvature{0.0}; ///< Minimum curvature along the edge
    double m_maxCurvature{0.0}; ///< Maximum curvature along the edge

    // Sizing hints
    double m_suggestedDivisions{10.0}; ///< Suggested number of divisions

    bool m_isDegenerated{false}; ///< Whether the edge is degenerated
    bool m_isClosed{false};      ///< Whether the edge forms a closed loop
};

/**
 * @brief Service for extracting mesh generation metadata from geometry
 */
class MeshMetadataExtractor {
public:
    MeshMetadataExtractor() = default;
    ~MeshMetadataExtractor() = default;

    /**
     * @brief Extract mesh metadata from a face entity
     * @param face_entity Source face entity
     * @return Face mesh metadata
     */
    [[nodiscard]] FaceMeshMetadata extractFaceMetadata(const FaceEntityPtr& face_entity) const;

    /**
     * @brief Extract mesh metadata from an edge entity
     * @param edge_entity Source edge entity
     * @return Edge mesh metadata
     */
    [[nodiscard]] EdgeMeshMetadata extractEdgeMetadata(const EdgeEntityPtr& edge_entity) const;

    /**
     * @brief Compute curvature at a point on a face
     * @param face_entity Source face entity
     * @param u U parameter
     * @param v V parameter
     * @return Curvature information at the point
     */
    [[nodiscard]] CurvatureInfo
    computeCurvatureAt(const FaceEntityPtr& face_entity, double u, double v) const;

    /**
     * @brief Suggest element size based on curvature
     * @param max_curvature Maximum curvature
     * @param chord_error Acceptable chord error
     * @return Suggested element size
     */
    [[nodiscard]] static double suggestElementSize(double max_curvature, double chord_error);

    /**
     * @brief Detect surface type from OCC geometry
     * @param face_entity Face entity to analyze
     * @return Detected surface type
     */
    [[nodiscard]] static SurfaceType detectSurfaceType(const FaceEntityPtr& face_entity);

    /**
     * @brief Detect curve type from OCC geometry
     * @param edge_entity Edge entity to analyze
     * @return Detected curve type
     */
    [[nodiscard]] static CurveType detectCurveType(const EdgeEntityPtr& edge_entity);
};

} // namespace OpenGeoLab::Geometry
