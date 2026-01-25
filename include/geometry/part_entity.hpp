#pragma once

#include "geometry_entity.hpp"

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab::Geometry {

class PartEntity;
using PartEntityPtr = std::shared_ptr<PartEntity>;

/**
 * @brief UI-level part entity representing an independent component.
 *
 * A PartEntity wraps a top-level OCC shape (solid/compound/etc.) and acts as the
 * root node for that component in the scene tree.
 */
class PartEntity : public GeometryEntity {
public:
    explicit PartEntity(const TopoDS_Shape& shape);
    ~PartEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::Part; }
    [[nodiscard]] const char* typeName() const override { return "Part"; }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_shape; }

    [[nodiscard]] const TopoDS_Shape& partShape() const { return m_shape; }

private:
    TopoDS_Shape m_shape;
};

} // namespace OpenGeoLab::Geometry
