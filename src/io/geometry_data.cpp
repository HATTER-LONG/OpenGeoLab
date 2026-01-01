/**
 * @file geometry_data.cpp
 * @brief Implementation of geometry data utilities
 */
#include "io/geometry_data.hpp"

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

} // namespace IO
} // namespace OpenGeoLab
