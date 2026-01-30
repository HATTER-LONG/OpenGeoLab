/**
 * @file mesh_metadata.cpp
 * @brief Implementation of mesh metadata extraction from OCC geometry
 */

#include "geometry/mesh_metadata.hpp"
#include "geometry/edge_entity.hpp"
#include "geometry/face_entity.hpp"
#include "util/logger.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <BRepLProp_SLProps.hxx>
#include <GProp_GProps.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomLProp_CLProps.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <ShapeAnalysis.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>


#include <cmath>

namespace OpenGeoLab::Geometry {

FaceMeshMetadata
MeshMetadataExtractor::extractFaceMetadata(const FaceEntityPtr& face_entity) const {
    FaceMeshMetadata metadata;

    if(!face_entity || !face_entity->hasShape()) {
        LOG_WARN("MeshMetadataExtractor: Cannot extract metadata from null face");
        return metadata;
    }

    metadata.m_entityId = face_entity->entityId();
    metadata.m_surfaceType = detectSurfaceType(face_entity);

    const TopoDS_Face& face = face_entity->face();

    try {
        // Get area
        metadata.m_area = face_entity->area();

        // Get parameter bounds
        face_entity->parameterBounds(metadata.m_uMin, metadata.m_uMax, metadata.m_vMin,
                                     metadata.m_vMax);

        // Get orientation
        metadata.m_isReversed = !face_entity->isForward();

        // Get hole count
        metadata.m_holeCount = face_entity->holeCount();

        // Count boundary edges
        for(TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next()) {
            ++metadata.m_boundaryEdgeCount;
        }

        // Calculate perimeter
        GProp_GProps props;
        BRepGProp::LinearProperties(face, props);
        metadata.m_perimeter = props.Mass();

        // Sample curvature at center
        double u_mid = (metadata.m_uMin + metadata.m_uMax) / 2.0;
        double v_mid = (metadata.m_vMin + metadata.m_vMax) / 2.0;

        auto curvature = computeCurvatureAt(face_entity, u_mid, v_mid);
        metadata.m_minCurvature = curvature.m_minCurvature;
        metadata.m_maxCurvature = curvature.m_maxCurvature;
        metadata.m_avgCurvature = curvature.m_meanCurvature;

        // Suggest element size based on curvature
        constexpr double default_chord_error = 0.01;
        if(std::abs(metadata.m_maxCurvature) > 1e-10) {
            metadata.m_suggestedElementSize =
                suggestElementSize(metadata.m_maxCurvature, default_chord_error);
        } else {
            // For flat surfaces, use a fraction of the characteristic length
            double char_length = std::sqrt(metadata.m_area);
            metadata.m_suggestedElementSize = char_length / 10.0;
        }

    } catch(const Standard_Failure& e) {
        LOG_WARN("MeshMetadataExtractor: Error extracting face metadata: {}",
                 e.GetMessageString() ? e.GetMessageString() : "Unknown");
    }

    return metadata;
}

EdgeMeshMetadata
MeshMetadataExtractor::extractEdgeMetadata(const EdgeEntityPtr& edge_entity) const {
    EdgeMeshMetadata metadata;

    if(!edge_entity || !edge_entity->hasShape()) {
        LOG_WARN("MeshMetadataExtractor: Cannot extract metadata from null edge");
        return metadata;
    }

    metadata.m_entityId = edge_entity->entityId();
    metadata.m_curveType = detectCurveType(edge_entity);

    try {
        metadata.m_length = edge_entity->length();
        edge_entity->parameterRange(metadata.m_firstParam, metadata.m_lastParam);
        metadata.m_isDegenerated = edge_entity->isDegenerated();
        metadata.m_isClosed = edge_entity->isClosed();

        // Sample curvature along the edge
        auto curve = edge_entity->curve();
        if(!curve.IsNull() && !metadata.m_isDegenerated) {
            GeomLProp_CLProps props(curve, 2, Precision::Confusion());

            constexpr int num_samples = 10;
            double param_step =
                (metadata.m_lastParam - metadata.m_firstParam) / static_cast<double>(num_samples);

            for(int i = 0; i <= num_samples; ++i) {
                double param = metadata.m_firstParam + i * param_step;
                props.SetParameter(param);

                if(props.IsTangentDefined()) {
                    double curvature = props.Curvature();
                    if(i == 0) {
                        metadata.m_minCurvature = curvature;
                        metadata.m_maxCurvature = curvature;
                    } else {
                        metadata.m_minCurvature = std::min(metadata.m_minCurvature, curvature);
                        metadata.m_maxCurvature = std::max(metadata.m_maxCurvature, curvature);
                    }
                }
            }
        }

        // Suggest number of divisions
        if(metadata.m_maxCurvature > 1e-10) {
            // More divisions for curved edges
            double radius_of_curvature = 1.0 / metadata.m_maxCurvature;
            double arc_element = radius_of_curvature * 0.1; // 0.1 radian per segment
            metadata.m_suggestedDivisions = std::max(5.0, metadata.m_length / arc_element);
        } else {
            // Straight edge - fewer divisions
            metadata.m_suggestedDivisions = std::max(2.0, metadata.m_length / 1.0);
        }

    } catch(const Standard_Failure& e) {
        LOG_WARN("MeshMetadataExtractor: Error extracting edge metadata: {}",
                 e.GetMessageString() ? e.GetMessageString() : "Unknown");
    }

    return metadata;
}

CurvatureInfo MeshMetadataExtractor::computeCurvatureAt(const FaceEntityPtr& face_entity,
                                                        double u,
                                                        double v) const {
    CurvatureInfo info;

    if(!face_entity || !face_entity->hasShape()) {
        return info;
    }

    try {
        BRepAdaptor_Surface adaptor(face_entity->face());
        BRepLProp_SLProps props(adaptor, u, v, 2, Precision::Confusion());

        if(props.IsCurvatureDefined()) {
            info.m_minCurvature = props.MinCurvature();
            info.m_maxCurvature = props.MaxCurvature();
            info.m_gaussianCurvature = props.GaussianCurvature();
            info.m_meanCurvature = props.MeanCurvature();

            gp_Dir d1, d2;
            props.CurvatureDirections(d1, d2);
            info.m_minDirection = Vector3D(d1.X(), d1.Y(), d1.Z());
            info.m_maxDirection = Vector3D(d2.X(), d2.Y(), d2.Z());
        }

    } catch(const Standard_Failure& e) {
        LOG_WARN("MeshMetadataExtractor: Error computing curvature: {}",
                 e.GetMessageString() ? e.GetMessageString() : "Unknown");
    }

    return info;
}

double MeshMetadataExtractor::suggestElementSize(double max_curvature, double chord_error) {
    if(std::abs(max_curvature) < 1e-10) {
        return 1.0; // Default for flat surfaces
    }

    // Based on chord error formula: h = R * (1 - cos(theta/2))
    // For small theta: h ≈ R * theta^2 / 8
    // Element size L ≈ R * theta
    // Therefore: L ≈ sqrt(8 * h * R) = sqrt(8 * chord_error / max_curvature)

    double radius = 1.0 / std::abs(max_curvature);
    return std::sqrt(8.0 * chord_error * radius);
}

SurfaceType MeshMetadataExtractor::detectSurfaceType(const FaceEntityPtr& face_entity) {
    if(!face_entity || !face_entity->hasShape()) {
        return SurfaceType::Unknown;
    }

    try {
        auto surface = face_entity->surface();
        if(surface.IsNull()) {
            return SurfaceType::Unknown;
        }

        if(surface->IsKind(STANDARD_TYPE(Geom_Plane))) {
            return SurfaceType::Plane;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_CylindricalSurface))) {
            return SurfaceType::Cylinder;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_ConicalSurface))) {
            return SurfaceType::Cone;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_SphericalSurface))) {
            return SurfaceType::Sphere;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_ToroidalSurface))) {
            return SurfaceType::Torus;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_BezierSurface))) {
            return SurfaceType::BezierSurface;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_BSplineSurface))) {
            return SurfaceType::BSplineSurface;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_SurfaceOfRevolution))) {
            return SurfaceType::SurfaceOfRevolution;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_SurfaceOfLinearExtrusion))) {
            return SurfaceType::SurfaceOfExtrusion;
        }
        if(surface->IsKind(STANDARD_TYPE(Geom_OffsetSurface))) {
            return SurfaceType::OffsetSurface;
        }

    } catch(const Standard_Failure&) {
        // Ignore and return unknown
    }

    return SurfaceType::Unknown;
}

CurveType MeshMetadataExtractor::detectCurveType(const EdgeEntityPtr& edge_entity) {
    if(!edge_entity || !edge_entity->hasShape()) {
        return CurveType::Unknown;
    }

    try {
        BRepAdaptor_Curve adaptor(edge_entity->edge());
        GeomAbs_CurveType occ_type = adaptor.GetType();

        switch(occ_type) {
        case GeomAbs_Line:
            return CurveType::Line;
        case GeomAbs_Circle:
            return CurveType::Circle;
        case GeomAbs_Ellipse:
            return CurveType::Ellipse;
        case GeomAbs_Hyperbola:
            return CurveType::Hyperbola;
        case GeomAbs_Parabola:
            return CurveType::Parabola;
        case GeomAbs_BezierCurve:
            return CurveType::BezierCurve;
        case GeomAbs_BSplineCurve:
            return CurveType::BSplineCurve;
        default:
            return CurveType::Unknown;
        }

    } catch(const Standard_Failure&) {
        // Ignore and return unknown
    }

    return CurveType::Unknown;
}

} // namespace OpenGeoLab::Geometry
