/// @file list_meshes_action.hpp
/// @brief Action to list all mesh entries in the MeshStore.

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

namespace OpenGeoLab::Mesh {

class MeshStore;

/// @brief Lists all mesh entries with summary information.
class OPENGEOLAB_MESH_EXPORT ListMeshesAction final : public Core::IAction {
public:
    explicit ListMeshesAction(MeshStore& store);
    ~ListMeshesAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"list_meshes"};

private:
    MeshStore& m_store;
};

} // namespace OpenGeoLab::Mesh
