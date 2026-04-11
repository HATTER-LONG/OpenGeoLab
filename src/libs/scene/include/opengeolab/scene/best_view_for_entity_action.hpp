/**
 * @file best_view_for_entity_action.hpp
 * @brief BestViewForEntityAction — optimal camera for viewing an entity
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Computes and applies the optimal camera to view a topology entity.
 *
 * Auto-computes viewing distance from the entity's bounding box diagonal
 * multiplied by a padding factor.
 */
class OPENGEOLAB_SCENE_EXPORT BestViewForEntityAction final : public Core::IAction {
public:
    explicit BestViewForEntityAction(SceneGraph& graph);
    ~BestViewForEntityAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"best_view_for_entity"};

    static constexpr float DEFAULT_PADDING = 1.5F;

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
