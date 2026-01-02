/**
 * @file geometry_converter.cpp
 * @brief Implementation of geometry model to render geometry converter
 */

#include "render/geometry_converter.hpp"

namespace OpenGeoLab {
namespace Render {

std::shared_ptr<RenderGeometry> GeometryConverter::convert(const Geometry::GeometryModel& model,
                                                           const QVector3D& defaultColor) {
    auto renderGeometry = std::make_shared<RenderGeometry>();

    if(model.isEmpty()) {
        return renderGeometry;
    }

    // Process all faces and extract render data
    size_t totalVertices = 0;
    size_t totalIndices = 0;

    // First pass: count total vertices and indices
    for(const auto& face : model.getFaces()) {
        totalVertices += face.m_meshVertices.size();
        totalIndices += face.m_meshIndices.size();
    }

    renderGeometry->vertices.reserve(totalVertices);
    renderGeometry->indices.reserve(totalIndices);

    uint32_t vertexOffset = 0;

    // Second pass: copy data with offset adjustment
    for(const auto& face : model.getFaces()) {
        // Copy vertices
        for(const auto& srcVertex : face.m_meshVertices) {
            RenderVertex vertex;
            vertex.position = QVector3D(static_cast<float>(srcVertex.m_position.m_x),
                                        static_cast<float>(srcVertex.m_position.m_y),
                                        static_cast<float>(srcVertex.m_position.m_z));
            vertex.normal = QVector3D(static_cast<float>(srcVertex.m_normal.m_x),
                                      static_cast<float>(srcVertex.m_normal.m_y),
                                      static_cast<float>(srcVertex.m_normal.m_z));
            vertex.color = defaultColor;
            renderGeometry->vertices.push_back(vertex);
        }

        // Copy indices with offset
        for(uint32_t index : face.m_meshIndices) {
            renderGeometry->indices.push_back(index + vertexOffset);
        }

        vertexOffset += static_cast<uint32_t>(face.m_meshVertices.size());
    }

    return renderGeometry;
}

} // namespace Render
} // namespace OpenGeoLab
