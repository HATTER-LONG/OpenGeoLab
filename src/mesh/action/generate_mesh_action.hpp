/**
 * @file generate_mesh_action.hpp
 * @brief Mesh action for generating a mesh from geometry entities via Gmsh
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "mesh/mesh_action.hpp"

#include <TopoDS_Shape.hxx>
#include <utility>
#include <vector>

namespace OpenGeoLab::Mesh {

/**
 * @brief Action for generating mesh entities from selected geometry entities
 *
 * Request parameters:
 * - action: "generate_mesh"
 * - entities: array of objects {"uid": <number>, "type": <string>}
 * - elementSize: number (global mesh size)
 * - meshDimension: number (2 or 3, default 2)
 * - elementType: string ("triangle", "quad", "auto", default "triangle")
 */
class GenerateMeshAction final : public MeshActionBase {
public:
    GenerateMeshAction() = default;
    ~GenerateMeshAction() override = default;

    [[nodiscard]] static std::string actionName() { return "generate_mesh"; }

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;

private:
    /**
     * @brief Build per-part compound shapes from selected face/part/solid entities.
     */
    std::vector<std::pair<Geometry::EntityUID, TopoDS_Shape>>
    createShapesByPartFromEntities(const Geometry::EntityRefSet& entities);

    /**
     * @brief Import shape into Gmsh, generate mesh, extract nodes/elements to MeshDocument.
     * @param part_shapes Per-part compound shapes to mesh
     * @param element_size Global element size
     * @param mesh_dimension 2 for surface mesh, 3 for volume mesh
     * @param element_type "triangle", "quad", or "auto"
     * @param progress_callback Progress reporting callback
     *
     * Gmsh must already be initialized before calling this method.
     */
    void importShapeToGmshAndMesh(
        const std::vector<std::pair<Geometry::EntityUID, TopoDS_Shape>>& part_shapes,
        double element_size,
        int mesh_dimension,
        const std::string& element_type,
        Util::ProgressCallback progress_callback);
};

class GenerateMeshActionFactory : public MeshActionFactory {
public:
    GenerateMeshActionFactory() = default;
    ~GenerateMeshActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<GenerateMeshAction>(); }
};

} // namespace OpenGeoLab::Mesh
