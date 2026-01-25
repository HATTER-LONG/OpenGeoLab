#include "geometry/part_entity.hpp"

namespace OpenGeoLab::Geometry {

PartEntity::PartEntity(const TopoDS_Shape& shape) : GeometryEntity(EntityType::Part), m_shape(shape) {}

} // namespace OpenGeoLab::Geometry
