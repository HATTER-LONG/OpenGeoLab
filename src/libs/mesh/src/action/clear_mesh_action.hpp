/**
 * @file clear_mesh_action.hpp
 * @brief ClearMeshAction — remove one or all stored meshes
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

namespace OpenGeoLab::Mesh {

class MeshStore;

class OPENGEOLAB_MESH_EXPORT ClearMeshAction final : public Core::IAction {
public:
    explicit ClearMeshAction(MeshStore& mesh_store);
    ~ClearMeshAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "clear_mesh";

private:
    MeshStore& m_meshStore;
};

} // namespace OpenGeoLab::Mesh
