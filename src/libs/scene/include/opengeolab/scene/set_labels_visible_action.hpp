/**
 * @file set_labels_visible_action.hpp
 * @brief SetLabelsVisibleAction — toggle label rendering visibility
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/**
 * @brief Enable or disable label rendering visibility.
 */
class OPENGEOLAB_SCENE_EXPORT SetLabelsVisibleAction final : public Core::IAction {
public:
    explicit SetLabelsVisibleAction(LabelManager& manager);
    ~SetLabelsVisibleAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_labels_visible";

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
