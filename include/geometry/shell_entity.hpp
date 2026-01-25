/**
 * @file shell_entity.hpp
 * @brief Shell (face collection) geometry entity
 *
 * ShellEntity wraps an OpenCASCADE TopoDS_Shell, representing a connected
 * set of faces forming a surface boundary.
 */

#pragma once

#include "geometry_entity.hpp"
#include <TopoDS_Shell.hxx>

namespace OpenGeoLab::Geometry {

class ShellEntity;
using ShellEntityPtr = std::shared_ptr<ShellEntity>;

/**
 * @brief Geometry entity representing a shell (connected face set)
 *
 * ShellEntity represents a connected set of faces that together form
 * a surface boundary. A closed shell can bound a solid volume.
 */
class ShellEntity : public GeometryEntity {
public:
    explicit ShellEntity(const TopoDS_Shell& shell);
    ~ShellEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::Shell; }

    [[nodiscard]] const char* typeName() const override { return "Shell"; }

    [[nodiscard]] bool canAddChildType(EntityType child_type) const override {
        return child_type == EntityType::Face;
    }

    [[nodiscard]] bool canAddParentType(EntityType parent_type) const override {
        return parent_type == EntityType::Solid;
    }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_shell; }

    /**
     * @brief Get the typed OCC shell
     * @return Const reference to TopoDS_Shell
     */
    [[nodiscard]] const TopoDS_Shell& shell() const { return m_shell; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Check if shell is closed (watertight)
     * @return true if closed
     */
    [[nodiscard]] bool isClosed() const;

    /**
     * @brief Get total surface area of shell
     * @return Sum of all face areas
     */
    [[nodiscard]] double area() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get number of faces in shell
     * @return Face count
     */
    [[nodiscard]] size_t faceCount() const;

private:
    TopoDS_Shell m_shell;
};
} // namespace OpenGeoLab::Geometry