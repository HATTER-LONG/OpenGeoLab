/**
 * @file comp_solid_entity.hpp
 * @brief Composite solid geometry entity
 *
 * CompSolidEntity wraps an OpenCASCADE TopoDS_CompSolid, representing
 * a set of solids sharing common faces.
 */

#pragma once

#include "geometry_entity.hpp"
#include <TopoDS_CompSolid.hxx>

namespace OpenGeoLab::Geometry {

class CompSolidEntity;
using CompSolidEntityPtr = std::shared_ptr<CompSolidEntity>;

/**
 * @brief Geometry entity representing a composite solid
 *
 * CompSolidEntity represents a set of solids that share common faces.
 * This is used for multi-body configurations where solids are connected.
 */
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