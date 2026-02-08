/**
 * @file compound_entity.hpp
 * @brief Compound (shape collection) geometry entity
 *
 * CompoundEntity wraps an OpenCASCADE TopoDS_Compound, representing
 * a general collection of shapes with no topological constraints.
 */

#pragma once

#include "geometry_entityImpl.hpp"
#include <TopoDS_Compound.hxx>

namespace OpenGeoLab::Geometry {

class CompoundEntity;
using CompoundEntityPtr = std::shared_ptr<CompoundEntity>;

/**
 * @brief Geometry entity representing a compound (shape collection)
 *
 * CompoundEntity represents a general collection of shapes without
 * topological constraints. Unlike CompSolid, compound shapes don't
 * need to share faces.
 */
class CompoundEntity : public GeometryEntityImpl {
public:
    explicit CompoundEntity(const TopoDS_Compound& compound);
    ~CompoundEntity() override = default;

    [[nodiscard]] bool canAddChildType(EntityType child_type) const override {
        return child_type != EntityType::None && child_type != EntityType::Part;
    }

    [[nodiscard]] bool canAddParentType(EntityType parent_type) const override {
        return parent_type == EntityType::Part || parent_type == EntityType::Compound;
    }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_compound; }

    [[nodiscard]] bool hasShape() const override { return !m_compound.IsNull(); }
    /**
     * @brief Get the typed OCC compound
     * @return Const reference to TopoDS_Compound
     */
    [[nodiscard]] const TopoDS_Compound& compound() const { return m_compound; }

private:
    TopoDS_Compound m_compound;
};
} // namespace OpenGeoLab::Geometry