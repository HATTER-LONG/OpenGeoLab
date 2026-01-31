/**
 * @file shell_entity.cpp
 * @brief Implementation of ShellEntity face collection operations
 */

#include "shell_entity.hpp"
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

namespace OpenGeoLab::Geometry {
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

} // namespace OpenGeoLab::Geometry