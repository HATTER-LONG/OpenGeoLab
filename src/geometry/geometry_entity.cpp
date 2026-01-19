/**
 * @file geometry_entity.cpp
 * @brief Implementation of geometry entity classes
 */

#include "geometry/geometry_entity.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GProp_GProps.hxx>
#include <GeomLProp_SLProps.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>


#include <algorithm>

namespace OpenGeoLab::Geometry {

// ============================================================================
// GeometryEntity Implementation
// ============================================================================

GeometryEntity::GeometryEntity() : m_id(generateEntityId()) {}

GeometryEntity::GeometryEntity(const TopoDS_Shape& shape)
    : m_id(generateEntityId()), m_shape(shape) {}

void GeometryEntity::setShape(const TopoDS_Shape& shape) { m_shape = shape; }

bool GeometryEntity::hasValidShape() const { return !m_shape.IsNull(); }

BoundingBox GeometryEntity::boundingBox() const {
    if(!hasValidShape()) {
        return BoundingBox();
    }

    Bnd_Box box;
    BRepBndLib::Add(m_shape, box);

    if(box.IsVoid()) {
        return BoundingBox();
    }

    double xmin = 0.0;
    double ymin = 0.0;
    double zmin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;
    double zmax = 0.0;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    return BoundingBox(Point3D(xmin, ymin, zmin), Point3D(xmax, ymax, zmax));
}

void GeometryEntity::addChild(GeometryEntityPtr child) {
    if(!child) {
        return;
    }
    child->setParent(weak_from_this());
    m_children.push_back(std::move(child));
}

bool GeometryEntity::removeChild(EntityId childId) {
    auto iter = std::remove_if(
        m_children.begin(), m_children.end(),
        [childId](const GeometryEntityPtr& child) { return child->id() == childId; });
    if(iter != m_children.end()) {
        m_children.erase(iter, m_children.end());
        return true;
    }
    return false;
}

GeometryEntityPtr GeometryEntity::findChild(EntityId childId) const {
    for(const auto& child : m_children) {
        if(child->id() == childId) {
            return child;
        }
        // Recursive search
        if(auto found = child->findChild(childId)) {
            return found;
        }
    }
    return nullptr;
}

// ============================================================================
// VertexEntity Implementation
// ============================================================================

VertexEntity::VertexEntity(const TopoDS_Shape& shape) : GeometryEntity(shape) { setName("Vertex"); }

Point3D VertexEntity::point() const {
    if(!hasValidShape()) {
        return Point3D();
    }

    const TopoDS_Vertex& vertex = TopoDS::Vertex(m_shape);
    gp_Pnt pnt = BRep_Tool::Pnt(vertex);
    return Point3D(pnt.X(), pnt.Y(), pnt.Z());
}

// ============================================================================
// EdgeEntity Implementation
// ============================================================================

EdgeEntity::EdgeEntity(const TopoDS_Shape& shape) : GeometryEntity(shape) { setName("Edge"); }

Point3D EdgeEntity::startPoint() const {
    if(!hasValidShape()) {
        return Point3D();
    }

    const TopoDS_Edge& edge = TopoDS::Edge(m_shape);
    TopoDS_Vertex vFirst;
    TopoDS_Vertex vLast;
    TopExp::Vertices(edge, vFirst, vLast);

    if(vFirst.IsNull()) {
        return Point3D();
    }

    gp_Pnt pnt = BRep_Tool::Pnt(vFirst);
    return Point3D(pnt.X(), pnt.Y(), pnt.Z());
}

Point3D EdgeEntity::endPoint() const {
    if(!hasValidShape()) {
        return Point3D();
    }

    const TopoDS_Edge& edge = TopoDS::Edge(m_shape);
    TopoDS_Vertex vFirst;
    TopoDS_Vertex vLast;
    TopExp::Vertices(edge, vFirst, vLast);

    if(vLast.IsNull()) {
        return Point3D();
    }

    gp_Pnt pnt = BRep_Tool::Pnt(vLast);
    return Point3D(pnt.X(), pnt.Y(), pnt.Z());
}

double EdgeEntity::length() const {
    if(!hasValidShape()) {
        return 0.0;
    }

    const TopoDS_Edge& edge = TopoDS::Edge(m_shape);
    BRepAdaptor_Curve curve(edge);
    return GCPnts_AbscissaPoint::Length(curve);
}

// ============================================================================
// FaceEntity Implementation
// ============================================================================

FaceEntity::FaceEntity(const TopoDS_Shape& shape) : GeometryEntity(shape) { setName("Face"); }

double FaceEntity::area() const {
    if(!hasValidShape()) {
        return 0.0;
    }

    GProp_GProps props;
    BRepGProp::SurfaceProperties(m_shape, props);
    return props.Mass();
}

Vector3D FaceEntity::normalAt(double u, double v) const {
    if(!hasValidShape()) {
        return Vector3D(0.0, 0.0, 1.0);
    }

    const TopoDS_Face& face = TopoDS::Face(m_shape);
    BRepAdaptor_Surface surface(face);

    GeomLProp_SLProps props(surface.Surface().Surface(), u, v, 1, Precision::Confusion());
    if(props.IsNormalDefined()) {
        gp_Dir normal = props.Normal();
        return Vector3D(normal.X(), normal.Y(), normal.Z());
    }

    return Vector3D(0.0, 0.0, 1.0);
}

// ============================================================================
// SolidEntity Implementation
// ============================================================================

SolidEntity::SolidEntity(const TopoDS_Shape& shape) : GeometryEntity(shape) { setName("Solid"); }

double SolidEntity::volume() const {
    if(!hasValidShape()) {
        return 0.0;
    }

    GProp_GProps props;
    BRepGProp::VolumeProperties(m_shape, props);
    return props.Mass();
}

std::vector<std::shared_ptr<FaceEntity>> SolidEntity::faces() const {
    std::vector<std::shared_ptr<FaceEntity>> result;

    if(!hasValidShape()) {
        return result;
    }

    for(TopExp_Explorer exp(m_shape, TopAbs_FACE); exp.More(); exp.Next()) {
        auto face = std::make_shared<FaceEntity>(exp.Current());
        result.push_back(face);
    }

    return result;
}

std::vector<std::shared_ptr<EdgeEntity>> SolidEntity::edges() const {
    std::vector<std::shared_ptr<EdgeEntity>> result;

    if(!hasValidShape()) {
        return result;
    }

    for(TopExp_Explorer exp(m_shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        auto edge = std::make_shared<EdgeEntity>(exp.Current());
        result.push_back(edge);
    }

    return result;
}

std::vector<std::shared_ptr<VertexEntity>> SolidEntity::vertices() const {
    std::vector<std::shared_ptr<VertexEntity>> result;

    if(!hasValidShape()) {
        return result;
    }

    for(TopExp_Explorer exp(m_shape, TopAbs_VERTEX); exp.More(); exp.Next()) {
        auto vertex = std::make_shared<VertexEntity>(exp.Current());
        result.push_back(vertex);
    }

    return result;
}

// ============================================================================
// PartEntity Implementation
// ============================================================================

PartEntity::PartEntity(const TopoDS_Shape& shape) : GeometryEntity(shape) { setName("Part"); }

void PartEntity::buildHierarchy() {
    if(!hasValidShape()) {
        return;
    }

    m_children.clear();

    // Extract solids
    for(TopExp_Explorer exp(m_shape, TopAbs_SOLID); exp.More(); exp.Next()) {
        auto solid = std::make_shared<SolidEntity>(exp.Current());
        addChild(solid);
    }

    // If no solids, extract faces directly
    if(m_children.empty()) {
        for(TopExp_Explorer exp(m_shape, TopAbs_FACE); exp.More(); exp.Next()) {
            auto face = std::make_shared<FaceEntity>(exp.Current());
            addChild(face);
        }
    }
}

std::vector<std::shared_ptr<SolidEntity>> PartEntity::solids() const {
    std::vector<std::shared_ptr<SolidEntity>> result;
    for(const auto& child : m_children) {
        if(auto solid = std::dynamic_pointer_cast<SolidEntity>(child)) {
            result.push_back(solid);
        }
    }
    return result;
}

std::vector<std::shared_ptr<FaceEntity>> PartEntity::faces() const {
    std::vector<std::shared_ptr<FaceEntity>> result;

    for(const auto& child : m_children) {
        if(auto face = std::dynamic_pointer_cast<FaceEntity>(child)) {
            result.push_back(face);
        } else if(auto solid = std::dynamic_pointer_cast<SolidEntity>(child)) {
            auto solidFaces = solid->faces();
            result.insert(result.end(), solidFaces.begin(), solidFaces.end());
        }
    }

    return result;
}

} // namespace OpenGeoLab::Geometry
