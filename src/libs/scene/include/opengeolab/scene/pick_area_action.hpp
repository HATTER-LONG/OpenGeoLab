/**
 * @file pick_area_action.hpp
 * @brief PickAreaAction — async box-select pick from command/Python
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class ViewportState;

/**
 * @brief Queue a box-select pick area request (async).
 *
 * Results appear in SelectionState after the next render frame.
 * Query with "scene.query_selection".
 *
 * Param: {"x0", "y0", "x1", "y1", "coordType": "normalized"|"pixel",
 *         "pickAction": "Add"|"Remove"}
 */
class OPENGEOLAB_SCENE_EXPORT PickAreaAction final : public Core::IAction {
public:
    explicit PickAreaAction(ViewportState& state);
    ~PickAreaAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "pick_area";

private:
    ViewportState& m_state;
};

} // namespace OpenGeoLab::Scene
