/**
 * @file deselect_action.hpp
 * @brief DeselectAction — remove entities from the selection state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SelectionState;

/**
 * @brief Remove entities from the current selection set.
 *
 * Param: {"entities": [{"shapeId": <int>, "type": <string>, "localId": <int>}, ...]}
 */
class OPENGEOLAB_SCENE_EXPORT DeselectAction final : public Core::IAction {
public:
    explicit DeselectAction(SelectionState& state);
    ~DeselectAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "deselect";

private:
    SelectionState& m_state;
};

} // namespace OpenGeoLab::Scene
