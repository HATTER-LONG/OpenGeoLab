/**
 * @file list_nodes_action.hpp
 * @brief ListNodesAction — query all scene nodes with visibility state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Query all scene nodes (excluding root) with id, name, visibility, parentId.
 */
class OPENGEOLAB_SCENE_EXPORT ListNodesAction final : public Core::IAction {
public:
    explicit ListNodesAction(const SceneGraph& graph);
    ~ListNodesAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"list_nodes"};

private:
    const SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
