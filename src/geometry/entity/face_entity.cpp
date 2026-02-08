/**
 * @file face_entity.cpp
 * @brief Implementation of FaceEntity surface operations
 */

#include "face_entity.hpp"
#include "../geometry_documentImpl.hpp"
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#include <unordered_set>

namespace OpenGeoLab::Geometry {
FaceEntity::FaceEntity(const TopoDS_Face& face)
    : GeometryEntityImpl(EntityType::Face), m_face(face) {}

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
    gp_Vec d1u, d1v;
    surf->D1(u, v, p, d1u, d1v);

    gp_Vec normal = d1u.Crossed(d1v);
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
    TopoDS_Wire outer_w = BRepTools::OuterWire(m_face);
    if(outer_w.IsNull()) {
        return nullptr;
    }

    const auto doc = document();
    if(!doc) {
        return nullptr;
    }

    for(const auto child_id : doc->relationships().directChildren(*this)) {
        const auto child = doc->findById(child_id);
        if(auto wire_entity = std::dynamic_pointer_cast<WireEntity>(child)) {
            if(wire_entity->wire().IsSame(outer_w)) {
                return wire_entity;
            }
        }
    }

    return nullptr;
}

std::vector<WireEntityPtr> FaceEntity::allWires() const {
    std::vector<WireEntityPtr> wires;

    const auto doc = document();
    if(!doc) {
        return wires;
    }

    for(const auto child_id : doc->relationships().directChildren(*this)) {
        const auto child = doc->findById(child_id);
        if(auto wire_entity = std::dynamic_pointer_cast<WireEntity>(child)) {
            wires.push_back(wire_entity);
        }
    }
    return wires;
}
size_t FaceEntity::holeCount() const {
    size_t wire_count = 0;
    for(TopExp_Explorer exp(m_face, TopAbs_WIRE); exp.More(); exp.Next()) {
        ++wire_count;
    }
    return (wire_count > 0) ? wire_count - 1 : 0; // Subtract outer wire
}

std::vector<FaceEntityPtr> FaceEntity::adjacentFaces() const {
    std::vector<FaceEntityPtr> result;

    const auto doc = document();
    if(!doc) {
        return result;
    }

    const auto parent_ids = doc->relationships().directParents(*this);
    if(parent_ids.empty()) {
        return result;
    }

    // Collect edges of this face
    TopTools_IndexedMapOfShape my_edges;
    TopExp::MapShapes(m_face, TopAbs_EDGE, my_edges);

    std::unordered_set<EntityId> added;
    added.insert(entityId());

    // Check sibling faces across all parents.
    for(const auto parent_id : parent_ids) {
        for(const auto sibling_id : doc->relationships().directChildren(parent_id)) {
            if(!added.insert(sibling_id).second) {
                continue;
            }

            const auto sibling = doc->findById(sibling_id);
            if(!sibling) {
                continue;
            }

            if(auto face_entity = std::dynamic_pointer_cast<FaceEntity>(sibling)) {
                // Check if any edge is shared
                for(TopExp_Explorer exp(face_entity->face(), TopAbs_EDGE); exp.More(); exp.Next()) {
                    if(my_edges.Contains(exp.Current())) {
                        result.push_back(face_entity);
                        break;
                    }
                }
            }
        }
    }

    return result;
}
} // namespace OpenGeoLab::Geometry