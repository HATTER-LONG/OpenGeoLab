/**
 * @file fit_to_scene_action.hpp
 * @brief FitToSceneAction — frame camera to show entire scene
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Fit camera to the scene bounding box.
 *
 * Param: {} (no parameters required)
 */
class OPENGEOLAB_SCENE_EXPORT FitToSceneAction final : public Core::IAction {
public:
    explicit FitToSceneAction(SceneGraph& graph);
    ~FitToSceneAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "fit_to_scene";

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
