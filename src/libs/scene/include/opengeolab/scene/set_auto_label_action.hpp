/**
 * @file set_auto_label_action.hpp
 * @brief SetAutoLabelAction — toggle automatic label creation
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/**
 * @brief Enable or disable automatic label creation on selection.
 */
class OPENGEOLAB_SCENE_EXPORT SetAutoLabelAction final : public Core::IAction {
public:
    explicit SetAutoLabelAction(LabelManager& manager);
    ~SetAutoLabelAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_auto_label";

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
