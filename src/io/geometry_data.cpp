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
    oss << "Parts: " << parts.size() << ", Solids: " << solids.size() << ", Faces: " << faces.size()
        << ", Edges: " << edges.size() << ", Vertices: " << vertices.size();
    return oss.str();
}

bool GeometryData::isEmpty() const {
    return parts.empty() && solids.empty() && faces.empty() && edges.empty() && vertices.empty();
}

void GeometryData::storeToGeometryStore() const {
    auto model = std::make_shared<Geometry::GeometryModel>();

    // Copy parts
    for(const auto& p : parts) {
        model->addPart(p);
    }

    // Copy solids
    for(const auto& s : solids) {
        model->addSolid(s);
    }

    // Copy faces
    for(const auto& f : faces) {
        model->addFace(f);
    }

    // Copy edges
    for(const auto& e : edges) {
        model->addEdge(e);
    }

    // Copy vertices
    for(const auto& v : vertices) {
        model->addVertex(v);
    }

    Geometry::GeometryStore::instance().setModel(model);
}

} // namespace IO
} // namespace OpenGeoLab
