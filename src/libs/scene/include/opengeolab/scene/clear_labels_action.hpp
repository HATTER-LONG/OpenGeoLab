/**
 * @file clear_labels_action.hpp
 * @brief ClearLabelsAction — clear all active labels
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/**
 * @brief Remove every active label from the label manager.
 */
class OPENGEOLAB_SCENE_EXPORT ClearLabelsAction final : public Core::IAction {
public:
    explicit ClearLabelsAction(LabelManager& manager);
    ~ClearLabelsAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "clear_labels";

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
