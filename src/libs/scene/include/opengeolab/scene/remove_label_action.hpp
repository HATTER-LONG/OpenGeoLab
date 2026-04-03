/**
 * @file remove_label_action.hpp
 * @brief RemoveLabelAction — remove a label by entity reference
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/**
 * @brief Remove the label bound to a specific entity, if present.
 */
class OPENGEOLAB_SCENE_EXPORT RemoveLabelAction final : public Core::IAction {
public:
    explicit RemoveLabelAction(LabelManager& manager);
    ~RemoveLabelAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "remove_label";

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
