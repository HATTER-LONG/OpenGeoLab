/**
 * @file compound_entity.cpp
 * @brief Implementation of CompoundEntity
 */

#include "compound_entity.hpp"

namespace OpenGeoLab::Geometry {

CompoundEntity::CompoundEntity(const TopoDS_Compound& compound)
    : GeometryEntityImpl(EntityType::Compound), m_compound(compound) {}

} // namespace OpenGeoLab::Geometry
