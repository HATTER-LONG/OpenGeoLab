/**
 * @file set_visibility_action.hpp
 * @brief SetVisibilityAction — batch set scene node visibility
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Batch-set visibility of scene nodes.
 *
 * Param: {"nodes": [{"nodeId": <int>, "visible": <bool>}, ...]}
 * Nodes not found are counted in "skipped" (no failure).
 */
class OPENGEOLAB_SCENE_EXPORT SetVisibilityAction final : public Core::IAction {
public:
    explicit SetVisibilityAction(SceneGraph& graph);
    ~SetVisibilityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"set_visibility"};

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
