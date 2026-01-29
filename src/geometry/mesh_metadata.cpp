/**
 * @file mesh_metadata.cpp
 * @brief Implementation of mesh metadata structures
 */

#include "geometry/mesh_metadata.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace OpenGeoLab::Geometry {

// =============================================================================
// SurfaceCurvature Implementation
// =============================================================================

double SurfaceCurvature::characteristicLength() const {
    const double max_abs_curvature = std::max(std::fabs(m_minCurvature), std::fabs(m_maxCurvature));
    if(max_abs_curvature < 1e-10) {
        return std::numeric_limits<double>::infinity();
    }
    return 1.0 / max_abs_curvature;
}

// =============================================================================
// EdgeMetadata Implementation
// =============================================================================

size_t EdgeMetadata::suggestSegmentCount(double base_size, double curvature_factor) const {
    if(m_isDegenerate || m_length < 1e-10) {
        return 1;
    }

    // Base count from length
    size_t base_count = static_cast<size_t>(std::ceil(m_length / base_size));

    // Refine based on curvature
    if(m_maxCurvature > 1e-10) {
        double curvature_size = curvature_factor / m_maxCurvature;
        size_t curvature_count = static_cast<size_t>(std::ceil(m_length / curvature_size));
        base_count = std::max(base_count, curvature_count);
    }

    // Ensure at least 1 segment
    return std::max(base_count, size_t{1});
}

// =============================================================================
// FaceMetadata Implementation
// =============================================================================

double FaceMetadata::suggestMeshSize(double base_size, double curvature_factor) const {
    double suggested_size = base_size;

    // Refine based on curvature
    const double max_abs_curvature = std::max(std::fabs(m_minCurvature), std::fabs(m_maxCurvature));
    if(max_abs_curvature > 1e-10) {
        double curvature_size = curvature_factor / max_abs_curvature;
        suggested_size = std::min(suggested_size, curvature_size);
    }

    // Don't go smaller than area allows
    double area_based_size = std::sqrt(m_area / 100.0); // Aim for ~100 elements
    suggested_size = std::max(suggested_size, area_based_size);

    return suggested_size;
}

SurfaceCurvature FaceMetadata::curvatureAt(double /*u*/, double /*v*/) const {
    // This is a placeholder - actual implementation would need access to the OCC surface
    // For now, return the face's overall curvature bounds
    SurfaceCurvature result;
    result.m_minCurvature = m_minCurvature;
    result.m_maxCurvature = m_maxCurvature;
    result.m_gaussianCurvature = m_minCurvature * m_maxCurvature;
    result.m_meanCurvature = (m_minCurvature + m_maxCurvature) / 2.0;
    return result;
}

// =============================================================================
// SolidMetadata Implementation
// =============================================================================

double SolidMetadata::characteristicLength() const {
    if(m_volume > 1e-20) {
        return std::cbrt(m_volume);
    }
    return m_boundingBox.diagonal();
}

// =============================================================================
// PartMeshMetadata Implementation
// =============================================================================

double PartMeshMetadata::suggestGlobalMeshSize(double elements_per_characteristic) const {
    if(m_characteristicLength > 1e-10) {
        return m_characteristicLength / elements_per_characteristic;
    }
    return m_boundingBox.diagonal() / elements_per_characteristic;
}

std::vector<EntityId> PartMeshMetadata::getAllFaceIds() const {
    std::vector<EntityId> ids;
    ids.reserve(m_faces.size());
    for(const auto& face : m_faces) {
        ids.push_back(face.m_entityId);
    }
    return ids;
}

std::vector<EntityId> PartMeshMetadata::getAllEdgeIds() const {
    std::vector<EntityId> ids;
    ids.reserve(m_edges.size());
    for(const auto& edge : m_edges) {
        ids.push_back(edge.m_entityId);
    }
    return ids;
}

// =============================================================================
// DocumentMeshMetadata Implementation
// =============================================================================

void DocumentMeshMetadata::updateSceneBoundingBox() {
    m_sceneBoundingBox = BoundingBox3D();
    for(const auto& part : m_parts) {
        if(part && part->m_boundingBox.isValid()) {
            m_sceneBoundingBox.expand(part->m_boundingBox);
        }
    }
}

double DocumentMeshMetadata::suggestGlobalMeshSize(double elements_per_characteristic) const {
    double diagonal = m_sceneBoundingBox.diagonal();
    if(diagonal < 1e-10) {
        return 1.0; // Default fallback
    }
    return diagonal / elements_per_characteristic;
}

} // namespace OpenGeoLab::Geometry
