/**
 * @file geometry_entity.cpp
 * @brief Implementation of geometry entity hierarchy
 */

#include "geometry/geometry_entity.hpp"

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <ShapeAnalysis_Wire.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>

#include <algorithm>

namespace OpenGeoLab::Geometry {

// =============================================================================
// GeometryEntity Base Implementation
// =============================================================================

GeometryEntity::GeometryEntity(EntityType type)
    : m_entityId(generateEntityId()), m_entityUID(generateEntityUID(type)) {}

void GeometryEntity::addChild(const GeometryEntityPtr& child) {
    if(child) {
        m_children.push_back(child);
        child->setParent(weak_from_this());
    }
}

bool GeometryEntity::removeChild(const GeometryEntityPtr& child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if(it != m_children.end()) {
        (*it)->setParent(GeometryEntityWeakPtr{});
        m_children.erase(it);
        return true;
    }
    return false;
}

const BoundingBox3D& GeometryEntity::boundingBox() const {
    if(!m_boundingBoxValid) {
        computeBoundingBox();
    }
    return m_boundingBox;
}

void GeometryEntity::computeBoundingBox() const {
    m_boundingBox = BoundingBox3D{};

    if(shape().IsNull()) {
        m_boundingBoxValid = true;
        return;
    }

    Bnd_Box occ_box;
    BRepBndLib::Add(shape(), occ_box);

    if(!occ_box.IsVoid()) {
        double xmin, ymin, zmin, xmax, ymax, zmax;
        occ_box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        m_boundingBox.m_min = Point3D(xmin, ymin, zmin);
        m_boundingBox.m_max = Point3D(xmax, ymax, zmax);
    }

    m_boundingBoxValid = true;
}

// =============================================================================
// VertexEntity Implementation
// =============================================================================

VertexEntity::VertexEntity(const TopoDS_Vertex& vertex)
    : GeometryEntity(EntityType::Vertex), m_vertex(vertex) {}

Point3D VertexEntity::point() const {
    gp_Pnt p = BRep_Tool::Pnt(m_vertex);
    return Point3D(p.X(), p.Y(), p.Z());
}

gp_Pnt VertexEntity::occPoint() const { return BRep_Tool::Pnt(m_vertex); }

std::vector<EdgeEntityPtr> VertexEntity::connectedEdges() const {
    std::vector<EdgeEntityPtr> result;

    // Get parent to search for sibling edges
    auto parentPtr = m_parent.lock();
    if(!parentPtr) {
        return result;
    }

    // Search through siblings for edges that contain this vertex
    for(const auto& sibling : parentPtr->children()) {
        if(auto edgeEntity = std::dynamic_pointer_cast<EdgeEntity>(sibling)) {
            // Check if this vertex belongs to the edge
            for(TopExp_Explorer exp(edgeEntity->edge(), TopAbs_VERTEX); exp.More(); exp.Next()) {
                if(exp.Current().IsSame(m_vertex)) {
                    result.push_back(edgeEntity);
                    break;
                }
            }
        }
    }

    return result;
}

size_t VertexEntity::edgeCount() const { return connectedEdges().size(); }

double VertexEntity::distanceTo(const VertexEntity& other) const {
    return point().distanceTo(other.point());
}

double VertexEntity::distanceTo(const Point3D& pt) const { return point().distanceTo(pt); }

double VertexEntity::distanceTo(const EdgeEntity& edge) const { return edge.distanceTo(*this); }

// =============================================================================
// EdgeEntity Implementation
// =============================================================================

EdgeEntity::EdgeEntity(const TopoDS_Edge& edge) : GeometryEntity(EntityType::Edge), m_edge(edge) {}

Handle(Geom_Curve) EdgeEntity::curve() const {
    double first, last;
    return BRep_Tool::Curve(m_edge, first, last);
}

void EdgeEntity::parameterRange(double& first, double& last) const {
    BRep_Tool::Range(m_edge, first, last);
}

Point3D EdgeEntity::pointAt(double u) const {
    Handle(Geom_Curve) crv = curve();
    if(crv.IsNull()) {
        return Point3D();
    }
    gp_Pnt p = crv->Value(u);
    return Point3D(p.X(), p.Y(), p.Z());
}

Vector3D EdgeEntity::tangentAt(double u) const {
    Handle(Geom_Curve) crv = curve();
    if(crv.IsNull()) {
        return Vector3D();
    }

    gp_Pnt p;
    gp_Vec tangent;
    crv->D1(u, p, tangent);

    if(tangent.Magnitude() > 1e-10) {
        tangent.Normalize();
        return Vector3D(tangent.X(), tangent.Y(), tangent.Z());
    }

    return Vector3D();
}

double EdgeEntity::length() const {
    GProp_GProps props;
    BRepGProp::LinearProperties(m_edge, props);
    return props.Mass();
}

bool EdgeEntity::isDegenerated() const { return BRep_Tool::Degenerated(m_edge); }

bool EdgeEntity::isClosed() const {
    TopoDS_Vertex v1, v2;
    TopExp::Vertices(m_edge, v1, v2);
    return v1.IsSame(v2);
}

Point3D EdgeEntity::startPoint() const {
    double first, last;
    parameterRange(first, last);
    return pointAt(first);
}

Point3D EdgeEntity::endPoint() const {
    double first, last;
    parameterRange(first, last);
    return pointAt(last);
}

Point3D EdgeEntity::midPoint() const {
    double first, last;
    parameterRange(first, last);
    return pointAt((first + last) / 2.0);
}

VertexEntityPtr EdgeEntity::startVertex() const {
    TopoDS_Vertex v1, v2;
    TopExp::Vertices(m_edge, v1, v2);

    // Search in children for matching vertex
    for(const auto& child : m_children) {
        if(auto vertexEntity = std::dynamic_pointer_cast<VertexEntity>(child)) {
            if(vertexEntity->vertex().IsSame(v1)) {
                return vertexEntity;
            }
        }
    }

    return nullptr;
}

VertexEntityPtr EdgeEntity::endVertex() const {
    TopoDS_Vertex v1, v2;
    TopExp::Vertices(m_edge, v1, v2);

    // Search in children for matching vertex
    for(const auto& child : m_children) {
        if(auto vertexEntity = std::dynamic_pointer_cast<VertexEntity>(child)) {
            if(vertexEntity->vertex().IsSame(v2)) {
                return vertexEntity;
            }
        }
    }

    return nullptr;
}

std::vector<VertexEntityPtr> EdgeEntity::getVertices() const {
    std::vector<VertexEntityPtr> result;

    TopoDS_Vertex v1, v2;
    TopExp::Vertices(m_edge, v1, v2);

    for(const auto& child : m_children) {
        if(auto vertexEntity = std::dynamic_pointer_cast<VertexEntity>(child)) {
            if(vertexEntity->vertex().IsSame(v1) || vertexEntity->vertex().IsSame(v2)) {
                result.push_back(vertexEntity);
            }
        }
    }

    return result;
}

std::vector<FaceEntityPtr> EdgeEntity::adjacentFaces() const {
    std::vector<FaceEntityPtr> result;

    // Navigate up the hierarchy to find faces
    auto parentPtr = m_parent.lock();
    while(parentPtr) {
        for(const auto& child : parentPtr->children()) {
            if(auto faceEntity = std::dynamic_pointer_cast<FaceEntity>(child)) {
                // Check if this edge is part of the face
                for(TopExp_Explorer exp(faceEntity->face(), TopAbs_EDGE); exp.More(); exp.Next()) {
                    if(exp.Current().IsSame(m_edge)) {
                        result.push_back(faceEntity);
                        break;
                    }
                }
            }
        }
        parentPtr = parentPtr->parent().lock();
    }

    return result;
}

double EdgeEntity::distanceTo(const VertexEntity& vertex) const {
    return distanceTo(vertex.point());
}

double EdgeEntity::distanceTo(const Point3D& pt) const {
    BRepExtrema_DistShapeShape distCalc;
    distCalc.LoadS1(m_edge);

    // Create a vertex from the point
    BRepBuilderAPI_MakeVertex mv(gp_Pnt(pt.m_x, pt.m_y, pt.m_z));
    distCalc.LoadS2(mv.Vertex());
    distCalc.Perform();

    if(distCalc.IsDone() && distCalc.NbSolution() > 0) {
        return distCalc.Value();
    }

    // Fallback: use projection
    Handle(Geom_Curve) crv = curve();
    if(!crv.IsNull()) {
        GeomAPI_ProjectPointOnCurve proj(gp_Pnt(pt.m_x, pt.m_y, pt.m_z), crv);
        if(proj.NbPoints() > 0) {
            return proj.LowerDistance();
        }
    }

    return std::numeric_limits<double>::max();
}

double EdgeEntity::distanceTo(const EdgeEntity& other) const {
    BRepExtrema_DistShapeShape distCalc(m_edge, other.edge());
    if(distCalc.IsDone() && distCalc.NbSolution() > 0) {
        return distCalc.Value();
    }
    return std::numeric_limits<double>::max();
}

Point3D EdgeEntity::closestPointTo(const Point3D& pt) const {
    Handle(Geom_Curve) crv = curve();
    if(crv.IsNull()) {
        return startPoint();
    }

    double first, last;
    parameterRange(first, last);

    GeomAPI_ProjectPointOnCurve proj(gp_Pnt(pt.m_x, pt.m_y, pt.m_z), crv, first, last);
    if(proj.NbPoints() > 0) {
        gp_Pnt closest = proj.NearestPoint();
        return Point3D(closest.X(), closest.Y(), closest.Z());
    }

    return startPoint();
}

double EdgeEntity::closestParameterTo(const Point3D& pt) const {
    Handle(Geom_Curve) crv = curve();
    if(crv.IsNull()) {
        double first, last;
        parameterRange(first, last);
        return first;
    }

    double first, last;
    parameterRange(first, last);

    GeomAPI_ProjectPointOnCurve proj(gp_Pnt(pt.m_x, pt.m_y, pt.m_z), crv, first, last);
    if(proj.NbPoints() > 0) {
        return proj.LowerDistanceParameter();
    }

    return first;
}

// =============================================================================
// WireEntity Implementation
// =============================================================================

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
    std::vector<EdgeEntityPtr> result;

    for(const auto& child : m_children) {
        if(auto edgeEntity = std::dynamic_pointer_cast<EdgeEntity>(child)) {
            result.push_back(edgeEntity);
        }
    }

    return result;
}

size_t WireEntity::edgeCount() const {
    size_t count = 0;
    for(TopExp_Explorer exp(m_wire, TopAbs_EDGE); exp.More(); exp.Next()) {
        ++count;
    }
    return count;
}

// =============================================================================
// FaceEntity Implementation
// =============================================================================

FaceEntity::FaceEntity(const TopoDS_Face& face) : GeometryEntity(EntityType::Face), m_face(face) {}

Handle(Geom_Surface) FaceEntity::surface() const { return BRep_Tool::Surface(m_face); }

void FaceEntity::parameterBounds(double& u_min, double& u_max, double& v_min, double& v_max) const {
    BRepTools::UVBounds(m_face, u_min, u_max, v_min, v_max);
}

Point3D FaceEntity::pointAt(double u, double v) const {
    Handle(Geom_Surface) surf = surface();
    if(surf.IsNull()) {
        return Point3D();
    }
    gp_Pnt p = surf->Value(u, v);
    return Point3D(p.X(), p.Y(), p.Z());
}

Vector3D FaceEntity::normalAt(double u, double v) const {
    Handle(Geom_Surface) surf = surface();
    if(surf.IsNull()) {
        return Vector3D();
    }

    gp_Pnt p;
    gp_Vec du, dv;
    surf->D1(u, v, p, du, dv);

    gp_Vec normal = du.Crossed(dv);
    if(normal.Magnitude() > 1e-10) {
        normal.Normalize();
        // Account for face orientation
        if(m_face.Orientation() == TopAbs_REVERSED) {
            normal.Reverse();
        }
        return Vector3D(normal.X(), normal.Y(), normal.Z());
    }

    return Vector3D();
}

double FaceEntity::area() const {
    GProp_GProps props;
    BRepGProp::SurfaceProperties(m_face, props);
    return props.Mass();
}

bool FaceEntity::isForward() const { return m_face.Orientation() == TopAbs_FORWARD; }

WireEntityPtr FaceEntity::outerWire() const {
    TopoDS_Wire outerW = BRepTools::OuterWire(m_face);
    if(outerW.IsNull()) {
        return nullptr;
    }

    for(const auto& child : m_children) {
        if(auto wireEntity = std::dynamic_pointer_cast<WireEntity>(child)) {
            if(wireEntity->wire().IsSame(outerW)) {
                return wireEntity;
            }
        }
    }

    return nullptr;
}

std::vector<WireEntityPtr> FaceEntity::allWires() const {
    std::vector<WireEntityPtr> result;

    for(const auto& child : m_children) {
        if(auto wireEntity = std::dynamic_pointer_cast<WireEntity>(child)) {
            result.push_back(wireEntity);
        }
    }

    return result;
}

size_t FaceEntity::holeCount() const {
    size_t wireCount = 0;
    for(TopExp_Explorer exp(m_face, TopAbs_WIRE); exp.More(); exp.Next()) {
        ++wireCount;
    }
    return (wireCount > 0) ? wireCount - 1 : 0; // Subtract outer wire
}

std::vector<FaceEntityPtr> FaceEntity::adjacentFaces() const {
    std::vector<FaceEntityPtr> result;

    auto parentPtr = m_parent.lock();
    if(!parentPtr) {
        return result;
    }

    // Collect edges of this face
    TopTools_IndexedMapOfShape myEdges;
    TopExp::MapShapes(m_face, TopAbs_EDGE, myEdges);

    // Check sibling faces
    for(const auto& sibling : parentPtr->children()) {
        if(sibling.get() == this)
            continue;

        if(auto faceEntity = std::dynamic_pointer_cast<FaceEntity>(sibling)) {
            // Check if any edge is shared
            for(TopExp_Explorer exp(faceEntity->face(), TopAbs_EDGE); exp.More(); exp.Next()) {
                if(myEdges.Contains(exp.Current())) {
                    result.push_back(faceEntity);
                    break;
                }
            }
        }
    }

    return result;
}

double FaceEntity::distanceTo(const Point3D& pt) const {
    BRepExtrema_DistShapeShape distCalc;
    distCalc.LoadS1(m_face);

    BRepBuilderAPI_MakeVertex mv(gp_Pnt(pt.m_x, pt.m_y, pt.m_z));
    distCalc.LoadS2(mv.Vertex());
    distCalc.Perform();

    if(distCalc.IsDone() && distCalc.NbSolution() > 0) {
        return distCalc.Value();
    }

    return std::numeric_limits<double>::max();
}

Point3D FaceEntity::closestPointTo(const Point3D& pt) const {
    Handle(Geom_Surface) surf = surface();
    if(surf.IsNull()) {
        return Point3D();
    }

    GeomAPI_ProjectPointOnSurf proj(gp_Pnt(pt.m_x, pt.m_y, pt.m_z), surf);
    if(proj.NbPoints() > 0) {
        gp_Pnt closest = proj.NearestPoint();
        return Point3D(closest.X(), closest.Y(), closest.Z());
    }

    return Point3D();
}

// =============================================================================
// ShellEntity Implementation
// =============================================================================

ShellEntity::ShellEntity(const TopoDS_Shell& shell)
    : GeometryEntity(EntityType::Shell), m_shell(shell) {}

bool ShellEntity::isClosed() const { return m_shell.Closed(); }

double ShellEntity::area() const {
    GProp_GProps props;
    BRepGProp::SurfaceProperties(m_shell, props);
    return props.Mass();
}

size_t ShellEntity::faceCount() const {
    size_t count = 0;
    for(TopExp_Explorer exp(m_shell, TopAbs_FACE); exp.More(); exp.Next()) {
        ++count;
    }
    return count;
}

// =============================================================================
// SolidEntity Implementation
// =============================================================================

SolidEntity::SolidEntity(const TopoDS_Solid& solid)
    : GeometryEntity(EntityType::Solid), m_solid(solid) {}

double SolidEntity::volume() const {
    GProp_GProps props;
    BRepGProp::VolumeProperties(m_solid, props);
    return props.Mass();
}

double SolidEntity::surfaceArea() const {
    GProp_GProps props;
    BRepGProp::SurfaceProperties(m_solid, props);
    return props.Mass();
}

Point3D SolidEntity::centerOfMass() const {
    GProp_GProps props;
    BRepGProp::VolumeProperties(m_solid, props);
    gp_Pnt center = props.CentreOfMass();
    return Point3D(center.X(), center.Y(), center.Z());
}

size_t SolidEntity::faceCount() const {
    size_t count = 0;
    for(TopExp_Explorer exp(m_solid, TopAbs_FACE); exp.More(); exp.Next()) {
        ++count;
    }
    return count;
}

size_t SolidEntity::edgeCount() const {
    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(m_solid, TopAbs_EDGE, edges);
    return edges.Extent();
}

size_t SolidEntity::vertexCount() const {
    TopTools_IndexedMapOfShape vertices;
    TopExp::MapShapes(m_solid, TopAbs_VERTEX, vertices);
    return vertices.Extent();
}

// =============================================================================
// CompoundEntity Implementation
// =============================================================================

CompoundEntity::CompoundEntity(const TopoDS_Compound& compound)
    : GeometryEntity(EntityType::Compound), m_compound(compound) {}

CompoundEntity::CompoundEntity(const TopoDS_Shape& shape)
    : GeometryEntity(EntityType::Compound), m_compound(TopoDS::Compound(shape)) {}

size_t CompoundEntity::subShapeCount() const {
    size_t count = 0;
    for(TopoDS_Iterator it(m_compound); it.More(); it.Next()) {
        ++count;
    }
    return count;
}

// =============================================================================
// Factory Function
// =============================================================================

GeometryEntityPtr createEntityFromShape(const TopoDS_Shape& shape) {
    if(shape.IsNull()) {
        return nullptr;
    }

    switch(shape.ShapeType()) {
    case TopAbs_VERTEX:
        return std::make_shared<VertexEntity>(TopoDS::Vertex(shape));
    case TopAbs_EDGE:
        return std::make_shared<EdgeEntity>(TopoDS::Edge(shape));
    case TopAbs_WIRE:
        return std::make_shared<WireEntity>(TopoDS::Wire(shape));
    case TopAbs_FACE:
        return std::make_shared<FaceEntity>(TopoDS::Face(shape));
    case TopAbs_SHELL:
        return std::make_shared<ShellEntity>(TopoDS::Shell(shape));
    case TopAbs_SOLID:
        return std::make_shared<SolidEntity>(TopoDS::Solid(shape));
    case TopAbs_COMPOUND:
    case TopAbs_COMPSOLID:
        return std::make_shared<CompoundEntity>(shape);
    default:
        return nullptr;
    }
}

} // namespace OpenGeoLab::Geometry
