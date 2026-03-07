/**
 * @file smooth_mesh_action.hpp
 * @brief Mesh action for smoothing selected mesh regions.
 */

#pragma once

#include "mesh/mesh_action.hpp"

namespace OpenGeoLab::Mesh {

class SmoothMeshAction final : public MeshActionBase {
public:
    SmoothMeshAction() = default;
    ~SmoothMeshAction() override = default;

    [[nodiscard]] static std::string actionName() { return "smooth_mesh"; }

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

class SmoothMeshActionFactory : public MeshActionFactory {
public:
    SmoothMeshActionFactory() = default;
    ~SmoothMeshActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<SmoothMeshAction>(); }
};

} // namespace OpenGeoLab::Mesh