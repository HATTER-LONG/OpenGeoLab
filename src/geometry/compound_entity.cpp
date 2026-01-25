#include "geometry/compound_entity.hpp"

namespace OpenGeoLab::Geometry {

CompoundEntity::CompoundEntity(const TopoDS_Compound& compound)
    : GeometryEntity(EntityType::Compound), m_compound(compound) {}

} // namespace OpenGeoLab::Geometry
