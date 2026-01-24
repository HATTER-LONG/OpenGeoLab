#pragma once

#include "geometry_entity.hpp"
#include <TopoDS_Compound.hxx>

namespace OpenGeoLab::Geometry {
class CompoundEntity;
using CompoundEntityPtr = std::shared_ptr<CompoundEntity>;

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