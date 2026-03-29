/// @file delete_mesh_action.hpp
/// @brief Action to remove a mesh entry from MeshStore.

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

namespace OpenGeoLab::Mesh {

class MeshStore;

/// @brief Deletes a mesh from the store by its ID.
class OPENGEOLAB_MESH_EXPORT DeleteMeshAction final : public Core::IAction {
public:
    explicit DeleteMeshAction(MeshStore& store);
    ~DeleteMeshAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"delete_mesh"};

private:
    MeshStore& m_store;
};

} // namespace OpenGeoLab::Mesh
