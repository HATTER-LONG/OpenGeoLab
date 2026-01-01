/**
 * @file geometry_model.cpp
 * @brief Implementation of geometry model container and store.
 */
#include "geometry/geometry_model.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace OpenGeoLab {
namespace Geometry {

void GeometryModel::clear() {
    m_parts.clear();
    m_solids.clear();
    m_faces.clear();
    m_edges.clear();
    m_vertices.clear();
    m_sourcePath.clear();
}

bool GeometryModel::isEmpty() const {
    return m_parts.empty() && m_solids.empty() && m_faces.empty() && m_edges.empty() &&
           m_vertices.empty();
}

std::string GeometryModel::getSummary() const {
    std::ostringstream oss;
    oss << "Parts: " << m_parts.size() << ", Solids: " << m_solids.size()
        << ", Faces: " << m_faces.size() << ", Edges: " << m_edges.size()
        << ", Vertices: " << m_vertices.size();
    return oss.str();
}

BoundingBox GeometryModel::computeBoundingBox() const {
    BoundingBox box;
    constexpr double max_val = std::numeric_limits<double>::max();
    constexpr double min_val = std::numeric_limits<double>::lowest();

    box.m_min = Point3D(max_val, max_val, max_val);
    box.m_max = Point3D(min_val, min_val, min_val);

    for(const auto& v : m_vertices) {
        box.m_min.m_x = std::min(box.m_min.m_x, v.m_position.m_x);
        box.m_min.m_y = std::min(box.m_min.m_y, v.m_position.m_y);
        box.m_min.m_z = std::min(box.m_min.m_z, v.m_position.m_z);
        box.m_max.m_x = std::max(box.m_max.m_x, v.m_position.m_x);
        box.m_max.m_y = std::max(box.m_max.m_y, v.m_position.m_y);
        box.m_max.m_z = std::max(box.m_max.m_z, v.m_position.m_z);
    }

    return box;
}

void GeometryModel::addPart(Part part) { m_parts.push_back(std::move(part)); }

const Part* GeometryModel::getPartById(uint32_t id) const {
    auto it =
        std::find_if(m_parts.begin(), m_parts.end(), [id](const Part& p) { return p.m_id == id; });
    return (it != m_parts.end()) ? &(*it) : nullptr;
}

void GeometryModel::addSolid(Solid solid) { m_solids.push_back(std::move(solid)); }

const Solid* GeometryModel::getSolidById(uint32_t id) const {
    auto it = std::find_if(m_solids.begin(), m_solids.end(),
                           [id](const Solid& s) { return s.m_id == id; });
    return (it != m_solids.end()) ? &(*it) : nullptr;
}

void GeometryModel::addFace(Face face) { m_faces.push_back(std::move(face)); }

const Face* GeometryModel::getFaceById(uint32_t id) const {
    auto it =
        std::find_if(m_faces.begin(), m_faces.end(), [id](const Face& f) { return f.m_id == id; });
    return (it != m_faces.end()) ? &(*it) : nullptr;
}

void GeometryModel::addEdge(Edge edge) { m_edges.push_back(std::move(edge)); }

const Edge* GeometryModel::getEdgeById(uint32_t id) const {
    auto it =
        std::find_if(m_edges.begin(), m_edges.end(), [id](const Edge& e) { return e.m_id == id; });
    return (it != m_edges.end()) ? &(*it) : nullptr;
}

void GeometryModel::addVertex(Vertex vertex) { m_vertices.push_back(std::move(vertex)); }

const Vertex* GeometryModel::getVertexById(uint32_t id) const {
    auto it = std::find_if(m_vertices.begin(), m_vertices.end(),
                           [id](const Vertex& v) { return v.m_id == id; });
    return (it != m_vertices.end()) ? &(*it) : nullptr;
}

// --- GeometryStore ---

GeometryStore& GeometryStore::instance() {
    static GeometryStore store;
    return store;
}

void GeometryStore::setModel(GeometryModelPtr model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_model = std::move(model);
}

GeometryModelPtr GeometryStore::getModel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_model;
}

void GeometryStore::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_model.reset();
}

bool GeometryStore::hasModel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_model != nullptr && !m_model->isEmpty();
}

} // namespace Geometry
} // namespace OpenGeoLab
