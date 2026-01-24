#pragma once

#include "geometry_entity.hpp"
#include <TopoDS_CompSolid.hxx>

namespace OpenGeoLab::Geometry {

class CompSolidEntity;
using CompSolidEntityPtr = std::shared_ptr<CompSolidEntity>;
class CompSolidEntity : public GeometryEntity {
public:
    explicit CompSolidEntity(const TopoDS_CompSolid& compsolid);
    ~CompSolidEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::CompSolid; }

    [[nodiscard]] const char* typeName() const override { return "CompSolid"; }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_compsolid; }

    /**
     * @brief Get the typed OCC compsolid
     * @return Const reference to TopoDS_CompSolid
     */
    [[nodiscard]] const TopoDS_CompSolid& compsolid() const { return m_compsolid; }

private:
    TopoDS_CompSolid m_compsolid;
};

} // namespace OpenGeoLab::Geometry