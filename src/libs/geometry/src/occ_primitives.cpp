/**
 * @file occ_primitives.cpp
 * @brief Implementation of OCC primitive factory functions.
 */

#include "occ_primitives.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace OpenGeoLab::Geometry {

TopoDS_Shape makeBox(std::array<double, 3> center, std::array<double, 3> size) {
    const gp_Pnt corner(center[0] - (size[0] / 2.0), center[1] - (size[1] / 2.0),
                        center[2] - (size[2] / 2.0));
    return BRepPrimAPI_MakeBox(corner, size[0], size[1], size[2]).Shape();
}

TopoDS_Shape makeCylinder(std::array<double, 3> center, double radius, double height) {
    const gp_Ax2 axis(gp_Pnt(center[0], center[1], center[2]), gp_Dir(0.0, 0.0, 1.0));
    return BRepPrimAPI_MakeCylinder(axis, radius, height).Shape();
}

TopoDS_Shape makeSphere(std::array<double, 3> center, double radius) {
    return BRepPrimAPI_MakeSphere(gp_Pnt(center[0], center[1], center[2]), radius).Shape();
}

TopoDS_Shape makeTorus(std::array<double, 3> center, double major_radius, double minor_radius) {
    const gp_Ax2 axis(gp_Pnt(center[0], center[1], center[2]), gp_Dir(0.0, 0.0, 1.0));
    return BRepPrimAPI_MakeTorus(axis, major_radius, minor_radius).Shape();
}

} // namespace OpenGeoLab::Geometry
