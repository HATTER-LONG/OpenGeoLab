/**
 * @file geometry_data.cpp
 * @brief Implementation of geometry data utilities.
 */
#include "io/geometry_data.hpp"
#include "geometry/geometry_model.hpp"

#include <sstream>

namespace OpenGeoLab {
namespace IO {

std::string GeometryData::getSummary() const {
    std::ostringstream oss;
    oss << "Parts: " << m_parts.size() << ", Solids: " << m_solids.size()
        << ", Faces: " << m_faces.size() << ", Edges: " << m_edges.size()
        << ", Vertices: " << m_vertices.size();
    return oss.str();
}

bool GeometryData::isEmpty() const {
    return m_parts.empty() && m_solids.empty() && m_faces.empty() && m_edges.empty() &&
           m_vertices.empty();
}

void GeometryData::storeToGeometryStore() const {
    auto model = std::make_shared<Geometry::GeometryModel>();

    // Copy parts
    for(const auto& p : m_parts) {
        model->addPart(p);
    }

    // Copy solids
    for(const auto& s : m_solids) {
        model->addSolid(s);
    }

    // Copy faces
    for(const auto& f : m_faces) {
        model->addFace(f);
    }

    // Copy edges
    for(const auto& e : m_edges) {
        model->addEdge(e);
    }

    // Copy vertices
    for(const auto& v : m_vertices) {
        model->addVertex(v);
    }

    Geometry::GeometryStore::instance().setModel(model);
}

} // namespace IO
} // namespace OpenGeoLab
