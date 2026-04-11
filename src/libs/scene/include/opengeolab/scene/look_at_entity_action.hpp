/**
 * @file look_at_entity_action.hpp
 * @brief LookAtEntityAction — point camera at a topology entity
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Points the camera at a specific topology entity.
 *
 * Keeps the current viewing distance and places the camera along
 * the entity's outward direction (face normal, or average face normal
 * for edges/vertices).
 */
class OPENGEOLAB_SCENE_EXPORT LookAtEntityAction final : public Core::IAction {
public:
    explicit LookAtEntityAction(SceneGraph& graph);
    ~LookAtEntityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"look_at_entity"};

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
