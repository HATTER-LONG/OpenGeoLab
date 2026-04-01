/**
 * @file select_action.hpp
 * @brief SelectAction — add entities to the selection state
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SelectionState;

/**
 * @brief Add entities to the current selection set.
 *
 * Param: {"entities": [{"shapeId": <int>, "type": <string>, "localId": <int>}, ...],
 *         "append": <bool>}
 */
class OPENGEOLAB_SCENE_EXPORT SelectAction final : public Core::IAction {
public:
    explicit SelectAction(SelectionState& state);
    ~SelectAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "select";

private:
    SelectionState& m_state;
};

} // namespace OpenGeoLab::Scene
