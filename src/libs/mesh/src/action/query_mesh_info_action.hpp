/**
 * @file query_mesh_info_action.hpp
 * @brief QueryMeshInfoAction — inspect stored mesh nodes and elements
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

namespace OpenGeoLab::Mesh {

class MeshStore;

/** @brief Query node/element counts and per-shape summaries from MeshStore. */
class OPENGEOLAB_MESH_EXPORT QueryMeshInfoAction final : public Core::IAction {
public:
    explicit QueryMeshInfoAction(const MeshStore& mesh_store);
    ~QueryMeshInfoAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "query_mesh_info";

private:
    const MeshStore& m_meshStore;
};

} // namespace OpenGeoLab::Mesh
