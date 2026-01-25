/**
 * @file compound_entity.hpp
 * @brief Compound (shape collection) geometry entity
 *
 * CompoundEntity wraps an OpenCASCADE TopoDS_Compound, representing
 * a general collection of shapes with no topological constraints.
 */

#pragma once

#include "geometry_entity.hpp"
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
class CompoundEntity : public GeometryEntity {
public:
    explicit CompoundEntity(const TopoDS_Compound& compound);
    ~CompoundEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::Compound; }

    [[nodiscard]] const char* typeName() const override { return "Compound"; }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_compound; }

    /**
     * @brief Get the typed OCC compound
     * @return Const reference to TopoDS_Compound
     */
    [[nodiscard]] const TopoDS_Compound& compound() const { return m_compound; }

private:
    TopoDS_Compound m_compound;
};
} // namespace OpenGeoLab::Geometry