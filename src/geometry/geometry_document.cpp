#include "geometry/geometry_document.hpp"

#include <TopoDS_Shape.hxx>

#include <unordered_set>

namespace OpenGeoLab::Geometry {

GeometryDocument::GeometryDocument(std::string name) : m_name(std::move(name)) {}

void GeometryDocument::clear() {
    m_root.reset();
    m_index.clear();
    m_name.clear();
    m_sourcePath.clear();
}

bool GeometryDocument::setRootEntity(const GeometryEntityPtr& root) {
    if(!root) {
        return false;
    }

    m_root = root;
    rebuildIndex();
    return true;
}

void GeometryDocument::setName(std::string name) { m_name = std::move(name); }

void GeometryDocument::setSourcePath(std::string source_path) {
    m_sourcePath = std::move(source_path);
}

void GeometryDocument::rebuildIndex() {
    m_index.clear();

    if(!m_root) {
        return;
    }

    std::vector<GeometryEntityPtr> stack;
    stack.push_back(m_root);

    std::unordered_set<EntityId> visited;

    while(!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();

        if(!current) {
            continue;
        }

        if(!visited.insert(current->entityId()).second) {
            continue;
        }

        (void)m_index.addEntity(current);

        const auto& children = current->children();
        for(const auto& child : children) {
            stack.push_back(child);
        }
    }
}

GeometryEntityPtr GeometryDocument::findById(EntityId entity_id) const {
    return m_index.findById(entity_id);
}

GeometryEntityPtr GeometryDocument::findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const {
    return m_index.findByUIDAndType(entity_uid, entity_type);
}

GeometryEntityPtr GeometryDocument::findByShape(const TopoDS_Shape& shape) const {
    return m_index.findByShape(shape);
}

std::vector<GeometryEntityPtr> GeometryDocument::parts() const {
    if(!m_root) {
        return {};
    }

    const auto& children = m_root->children();
    return std::vector<GeometryEntityPtr>(children.begin(), children.end());
}

} // namespace OpenGeoLab::Geometry
