/**
 * @file render_data.cpp
 * @brief Implementation of render data structures and utilities
 */

#include "geometry/render_data.hpp"

#include <cmath>

namespace OpenGeoLab::Geometry {

// =============================================================================
// RenderColor Implementation
// =============================================================================

RenderColor RenderColor::fromHSV(float h, float s, float v) {
    // Normalize hue to [0, 360)
    h = std::fmod(h, 360.0f);
    if(h < 0.0f) {
        h += 360.0f;
    }

    const float c = v * s; // Chroma
    const float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    const float m = v - c;

    float r = 0.0f, g = 0.0f, b = 0.0f;

    if(h < 60.0f) {
        r = c;
        g = x;
        b = 0.0f;
    } else if(h < 120.0f) {
        r = x;
        g = c;
        b = 0.0f;
    } else if(h < 180.0f) {
        r = 0.0f;
        g = c;
        b = x;
    } else if(h < 240.0f) {
        r = 0.0f;
        g = x;
        b = c;
    } else if(h < 300.0f) {
        r = x;
        g = 0.0f;
        b = c;
    } else {
        r = c;
        g = 0.0f;
        b = x;
    }

    return RenderColor(r + m, g + m, b + m, 1.0f);
}

RenderColor RenderColor::fromIndex(size_t index, float saturation, float value) {
    // Golden ratio for well-distributed hues
    constexpr float GOLDEN_RATIO = 0.618033988749895f;
    const float hue = std::fmod(static_cast<float>(index) * GOLDEN_RATIO, 1.0f) * 360.0f;
    return fromHSV(hue, saturation, value);
}

// =============================================================================
// PartRenderData Implementation
// =============================================================================

size_t PartRenderData::totalTriangleCount() const {
    size_t count = 0;
    for(const auto& face : m_faces) {
        count += face.triangleCount();
    }
    return count;
}

size_t PartRenderData::totalVertexCount() const {
    size_t count = 0;
    for(const auto& face : m_faces) {
        count += face.vertexCount();
    }
    return count;
}

size_t PartRenderData::totalEdgePointCount() const {
    size_t count = 0;
    for(const auto& edge : m_edges) {
        count += edge.m_points.size();
    }
    return count;
}

void PartRenderData::mergeToBuffers(std::vector<RenderVertex>& out_vertices,
                                    std::vector<uint32_t>& out_indices) const {
    // Reserve space
    size_t total_vertices = totalVertexCount();
    size_t total_indices = 0;
    for(const auto& face : m_faces) {
        total_indices += face.m_indices.size();
    }

    out_vertices.reserve(out_vertices.size() + total_vertices);
    out_indices.reserve(out_indices.size() + total_indices);

    uint32_t base_index = static_cast<uint32_t>(out_vertices.size());

    for(const auto& face : m_faces) {
        // Add vertices
        for(const auto& vertex : face.m_vertices) {
            out_vertices.push_back(vertex);
        }

        // Add indices with offset
        for(uint32_t idx : face.m_indices) {
            out_indices.push_back(base_index + idx);
        }

        base_index += static_cast<uint32_t>(face.m_vertices.size());
    }
}

std::vector<float> PartRenderData::getEdgeLineBuffer() const {
    std::vector<float> buffer;

    for(const auto& edge : m_edges) {
        if(edge.m_points.size() < 2) {
            continue;
        }

        // Create line segments between consecutive points
        for(size_t i = 0; i + 1 < edge.m_points.size(); ++i) {
            const auto& p1 = edge.m_points[i];
            const auto& p2 = edge.m_points[i + 1];

            // First point
            buffer.push_back(static_cast<float>(p1.m_x));
            buffer.push_back(static_cast<float>(p1.m_y));
            buffer.push_back(static_cast<float>(p1.m_z));

            // Second point
            buffer.push_back(static_cast<float>(p2.m_x));
            buffer.push_back(static_cast<float>(p2.m_y));
            buffer.push_back(static_cast<float>(p2.m_z));
        }
    }

    return buffer;
}

// =============================================================================
// DocumentRenderData Implementation
// =============================================================================

size_t DocumentRenderData::totalTriangleCount() const {
    size_t count = 0;
    for(const auto& part : m_parts) {
        if(part) {
            count += part->totalTriangleCount();
        }
    }
    return count;
}

void DocumentRenderData::updateSceneBoundingBox() {
    m_sceneBoundingBox = BoundingBox3D();
    for(const auto& part : m_parts) {
        if(part && part->m_boundingBox.isValid()) {
            m_sceneBoundingBox.expand(part->m_boundingBox);
        }
    }
}

} // namespace OpenGeoLab::Geometry
