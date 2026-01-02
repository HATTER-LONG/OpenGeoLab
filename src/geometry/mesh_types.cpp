/**
 * @file mesh_types.cpp
 * @brief Implementation of mesh data types
 */

#include "geometry/mesh_types.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace OpenGeoLab {
namespace Mesh {

// Use type alias for convenience
using Point3D = Geometry::Point3D;

void MeshData::clear() {
    m_nodes.clear();
    m_elements.clear();
    m_regions.clear();
    m_elementQualities.clear();
    m_qualitySummary = MeshQualitySummary();
}

const MeshNode* MeshData::getNodeById(uint32_t id) const {
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
                           [id](const MeshNode& n) { return n.m_id == id; });
    return (it != m_nodes.end()) ? &(*it) : nullptr;
}

const MeshElement* MeshData::getElementById(uint32_t id) const {
    auto it = std::find_if(m_elements.begin(), m_elements.end(),
                           [id](const MeshElement& e) { return e.m_id == id; });
    return (it != m_elements.end()) ? &(*it) : nullptr;
}

namespace {

// Helper to compute triangle quality
ElementQuality computeTriangleQuality(const MeshData& mesh, const MeshElement& element) {
    ElementQuality quality;
    quality.m_elementId = element.m_id;

    if(element.m_nodeIds.size() != 3) {
        quality.m_isValid = false;
        quality.m_quality = 0.0;
        return quality;
    }

    // Get node positions
    const MeshNode* n0 = mesh.getNodeById(element.m_nodeIds[0]);
    const MeshNode* n1 = mesh.getNodeById(element.m_nodeIds[1]);
    const MeshNode* n2 = mesh.getNodeById(element.m_nodeIds[2]);

    if(!n0 || !n1 || !n2) {
        quality.m_isValid = false;
        quality.m_quality = 0.0;
        return quality;
    }

    // Calculate edge lengths
    auto edgeLength = [](const Point3D& a, const Point3D& b) {
        double dx = b.m_x - a.m_x;
        double dy = b.m_y - a.m_y;
        double dz = b.m_z - a.m_z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    };

    double e0 = edgeLength(n0->m_position, n1->m_position);
    double e1 = edgeLength(n1->m_position, n2->m_position);
    double e2 = edgeLength(n2->m_position, n0->m_position);

    double minEdge = std::min({e0, e1, e2});
    double maxEdge = std::max({e0, e1, e2});

    // Aspect ratio
    quality.m_aspectRatio = (minEdge > 1e-10) ? (maxEdge / minEdge) : 1000.0;

    // Calculate angles using law of cosines
    auto calcAngle = [](double a, double b, double c) {
        double cosAngle = (a * a + b * b - c * c) / (2.0 * a * b);
        cosAngle = std::clamp(cosAngle, -1.0, 1.0);
        return std::acos(cosAngle) * 180.0 / M_PI;
    };

    double a0 = calcAngle(e0, e2, e1);
    double a1 = calcAngle(e0, e1, e2);
    double a2 = calcAngle(e1, e2, e0);

    quality.m_minAngle = std::min({a0, a1, a2});
    quality.m_maxAngle = std::max({a0, a1, a2});

    // Skewness (deviation from equilateral)
    double idealAngle = 60.0;
    double maxDeviation =
        std::max({std::abs(a0 - idealAngle), std::abs(a1 - idealAngle), std::abs(a2 - idealAngle)});
    quality.m_skewness = maxDeviation / 60.0;

    // Area-based Jacobian approximation
    double s = (e0 + e1 + e2) / 2.0;
    double area = std::sqrt(s * (s - e0) * (s - e1) * (s - e2));
    double idealArea = (std::sqrt(3.0) / 4.0) * maxEdge * maxEdge;
    quality.m_jacobian = (idealArea > 1e-10) ? (area / idealArea) : 0.0;

    // Overall quality (normalized aspect ratio inverse)
    quality.m_quality = 1.0 / std::max(1.0, quality.m_aspectRatio);

    // Valid if positive area and reasonable angles
    quality.m_isValid = (area > 1e-10) && (quality.m_minAngle > 0.1);

    return quality;
}

} // anonymous namespace

void MeshData::computeQuality() {
    m_elementQualities.clear();
    m_elementQualities.reserve(m_elements.size());

    m_qualitySummary = MeshQualitySummary();
    m_qualitySummary.totalElements = m_elements.size();

    double sumQuality = 0.0;
    double sumAspectRatio = 0.0;
    double minQ = 1.0, maxQ = 0.0;
    double minAR = 1000.0, maxAR = 1.0;

    for(const auto& element : m_elements) {
        ElementQuality eq;

        switch(element.m_type) {
        case ElementType::Triangle:
            eq = computeTriangleQuality(*this, element);
            break;
        default:
            // Placeholder for other element types
            eq.m_elementId = element.m_id;
            eq.m_quality = 1.0;
            eq.m_aspectRatio = 1.0;
            eq.m_isValid = true;
            break;
        }

        m_elementQualities.push_back(eq);

        if(eq.m_isValid) {
            m_qualitySummary.validElements++;
            sumQuality += eq.m_quality;
            sumAspectRatio += eq.m_aspectRatio;
            minQ = std::min(minQ, eq.m_quality);
            maxQ = std::max(maxQ, eq.m_quality);
            minAR = std::min(minAR, eq.m_aspectRatio);
            maxAR = std::max(maxAR, eq.m_aspectRatio);
        } else {
            m_qualitySummary.invalidElements++;
        }
    }

    if(m_qualitySummary.validElements > 0) {
        m_qualitySummary.avgQuality = sumQuality / m_qualitySummary.validElements;
        m_qualitySummary.avgAspectRatio = sumAspectRatio / m_qualitySummary.validElements;
        m_qualitySummary.minQuality = minQ;
        m_qualitySummary.maxQuality = maxQ;
        m_qualitySummary.minAspectRatio = minAR;
        m_qualitySummary.maxAspectRatio = maxAR;
    }
}

std::vector<uint32_t> MeshData::getPoorQualityElements(const QualityThresholds& thresholds) const {
    std::vector<uint32_t> poorElements;

    for(const auto& eq : m_elementQualities) {
        if(!eq.m_isValid || eq.m_quality < thresholds.minQuality ||
           eq.m_aspectRatio > thresholds.maxAspectRatio || eq.m_skewness > thresholds.maxSkewness ||
           eq.m_minAngle < thresholds.minAngle || eq.m_maxAngle > thresholds.maxAngle) {
            poorElements.push_back(eq.m_elementId);
        }
    }

    return poorElements;
}

std::string MeshData::getSummary() const {
    std::ostringstream oss;
    oss << "Mesh: " << m_nodes.size() << " nodes, " << m_elements.size() << " elements";

    if(!m_elementQualities.empty()) {
        oss << " | Quality: avg=" << m_qualitySummary.avgQuality
            << ", min=" << m_qualitySummary.minQuality;
        if(m_qualitySummary.invalidElements > 0) {
            oss << " (" << m_qualitySummary.invalidElements << " invalid)";
        }
    }

    return oss.str();
}

} // namespace Mesh
} // namespace OpenGeoLab
