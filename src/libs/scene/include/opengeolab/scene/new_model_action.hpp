/**
 * @file new_model_action.hpp
 * @brief NewModelAction — clear all geometry, scene state, and reset camera
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Reset the entire workspace to an empty state.
 *
 * Clears geometry shapes (via GeometryModule), selection, labels,
 * hover state, and resets the camera to the default position.
 * Shape removal cascades through GeometrySceneBridge to remove
 * scene nodes and topology data automatically.
 */
class OPENGEOLAB_SCENE_EXPORT NewModelAction final : public Core::IAction {
public:
    static constexpr std::string_view ACTION_NAME{"new_model"};

    NewModelAction(SceneGraph& graph, Kangaroo::Util::PluginComponentFactory& factory);
    ~NewModelAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    nlohmann::json execute(const nlohmann::json& param,
                           const Core::ProgressCallback& progress) override;

private:
    SceneGraph& m_graph;
    Kangaroo::Util::PluginComponentFactory& m_factory;
};

} // namespace OpenGeoLab::Scene
