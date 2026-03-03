/**
 * @file newmodel_action.hpp
 * @brief Action for creating a new empty mesh
 */

#pragma once

#include "mesh/mesh_action.hpp"

namespace OpenGeoLab::Mesh {

/**
 * @brief Action that clears the current MeshDocument (new empty mesh)
 */
class NewModelAction : public MeshActionBase {
public:
    NewModelAction() = default;
    ~NewModelAction() override = default;

    [[nodiscard]] static std::string actionName() { return "new_mesh"; }

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

class NewModelActionFactory : public MeshActionFactory {
public:
    NewModelActionFactory() = default;
    ~NewModelActionFactory() = default;
    tObjectPtr create() override { return std::make_unique<NewModelAction>(); }
};

} // namespace OpenGeoLab::Mesh
