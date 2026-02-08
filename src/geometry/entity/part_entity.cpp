/**
 * @file part_entity.cpp
 * @brief Implementation of PartEntity top-level component
 */

#include "part_entity.hpp"

namespace OpenGeoLab::Geometry {

PartEntity::PartEntity(const TopoDS_Shape& shape)
    : GeometryEntityImpl(EntityType::Part), m_shape(shape) {}

} // namespace OpenGeoLab::Geometry
