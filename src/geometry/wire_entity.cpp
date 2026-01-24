#include "geometry/wire_entity.hpp"
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <ShapeAnalysis_Wire.hxx>
#include <TopExp_Explorer.hxx>

namespace OpenGeoLab::Geometry {
WireEntity::WireEntity(const TopoDS_Wire& wire) : GeometryEntity(EntityType::Wire), m_wire(wire) {}

bool WireEntity::isClosed() const {
    ShapeAnalysis_Wire analyzer;
    analyzer.Load(m_wire);
    return analyzer.CheckClosed();
}

double WireEntity::length() const {
    GProp_GProps props;
    BRepGProp::LinearProperties(m_wire, props);
    return props.Mass();
}

std::vector<EdgeEntityPtr> WireEntity::orderedEdges() const {
    std::vector<EdgeEntityPtr> edges;
    for(const auto& child : m_children) {
        if(auto edge_entity = std::dynamic_pointer_cast<EdgeEntity>(child)) {
            edges.push_back(edge_entity);
        }
    }
    return edges;
}

size_t WireEntity::edgeCount() const {
    size_t count = 0;
    for(TopExp_Explorer exp(m_wire, TopAbs_EDGE); exp.More(); exp.Next()) {
        ++count;
    }
    return count;
}
} // namespace OpenGeoLab::Geometry