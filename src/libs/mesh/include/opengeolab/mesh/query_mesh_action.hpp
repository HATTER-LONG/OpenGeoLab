/// @file query_mesh_action.hpp
/// @brief Action to query detailed information about a mesh entry.

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

namespace OpenGeoLab::Mesh {

class MeshStore;

/// @brief Returns detailed information about a mesh (node count, element summary, bounding box).
class OPENGEOLAB_MESH_EXPORT QueryMeshAction final : public Core::IAction {
public:
    explicit QueryMeshAction(MeshStore& store);
    ~QueryMeshAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"query_mesh"};

private:
    MeshStore& m_store;
};

} // namespace OpenGeoLab::Mesh
