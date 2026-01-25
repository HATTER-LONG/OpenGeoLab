/**
 * @file part_entity.hpp
 * @brief Part entity representing a UI-level component
 *
 * PartEntity is the top-level container for imported or created geometry.
 * It serves as the root node in the entity hierarchy for a model component.
 */

#pragma once

#include "geometry_entity.hpp"

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab::Geometry {

class PartEntity;
using PartEntityPtr = std::shared_ptr<PartEntity>;

/**
 * @brief UI-level part entity representing an independent component
 *
 * PartEntity is the top-level entity for user-visible model components.
 * It wraps a TopoDS_Shape (solid, compound, etc.) and serves as the root
 * of the entity hierarchy for that component. Parts can contain multiple
 * sub-shapes (solids, faces, edges, etc.) organized in a parent-child tree.
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
