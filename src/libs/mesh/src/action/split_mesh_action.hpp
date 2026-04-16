/**
 * @file split_mesh_action.hpp
 * @brief SplitMeshAction — subdivide mesh elements by edge/node selection
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

namespace OpenGeoLab::Mesh {

class MeshStore;

/**
 * @brief Split mesh elements based on user-selected edges or nodes.
 */
class OPENGEOLAB_MESH_EXPORT SplitMeshAction final : public Core::IAction {
public:
    /**
     * @brief Construct a SplitMeshAction bound to the given store.
     * @param mesh_store Reference to the MeshStore; must outlive this action.
     */
    explicit SplitMeshAction(MeshStore& mesh_store);
    ~SplitMeshAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "split_mesh";

private:
    MeshStore& m_meshStore;
};

} // namespace OpenGeoLab::Mesh
