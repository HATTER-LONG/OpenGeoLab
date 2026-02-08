/**
 * @file generate_mesh_action.hpp
 * @brief Mesh action for generating a mesh from geometry entities via Gmsh
 */

#pragma once

#include "mesh/mesh_action.hpp"

namespace OpenGeoLab::Mesh {

/**
 * @brief Action for generating mesh entities from selected geometry entities
 *
 * Request parameters:
 * - action: "generate_mesh"
 * - entities: array of objects {"uid": <number>, "type": <string>} or {"id": <number>}
 * - elementSize: number (global mesh size)
 */
class GenerateMeshAction final : public MeshActionBase {
public:
    GenerateMeshAction() = default;
    ~GenerateMeshAction() override = default;

    [[nodiscard]] static std::string actionName() { return "generate_mesh"; }

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

class GenerateMeshActionFactory : public MeshActionFactory {
public:
    GenerateMeshActionFactory() = default;
    ~GenerateMeshActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<GenerateMeshAction>(); }
};

} // namespace OpenGeoLab::Mesh
