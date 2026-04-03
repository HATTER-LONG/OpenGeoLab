/**
 * @file add_label_action.hpp
 * @brief AddLabelAction — create or replace a label for an entity
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class LabelManager;

/**
 * @brief Create or replace a label using the canonical entity text and colors.
 */
class OPENGEOLAB_SCENE_EXPORT AddLabelAction final : public Core::IAction {
public:
    explicit AddLabelAction(LabelManager& manager);
    ~AddLabelAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "add_label";

private:
    LabelManager& m_manager;
};

} // namespace OpenGeoLab::Scene
