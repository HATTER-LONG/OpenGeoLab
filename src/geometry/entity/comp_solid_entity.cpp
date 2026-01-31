/**
 * @file comp_solid_entity.cpp
 * @brief Implementation of CompSolidEntity
 */

#include "comp_solid_entity.hpp"

namespace OpenGeoLab::Geometry {
CompSolidEntity::CompSolidEntity(const TopoDS_CompSolid& compsolid)
    : GeometryEntity(EntityType::CompSolid), m_compsolid(compsolid) {}
} // namespace OpenGeoLab::Geometry