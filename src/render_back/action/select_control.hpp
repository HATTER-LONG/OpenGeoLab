/**
 * @file select_control.hpp
 * @brief Render action for controlling viewport picking/selection state
 */

#pragma once

#include "render/render_action.hpp"

namespace OpenGeoLab::Render {

/**
 * @brief Render action that controls SelectManager picking mode and selection set
 *
 * This action is intended to be triggered from the UI layer via RenderService.
 */
class SelectControl : public RenderActionBase {
public:
    SelectControl() = default;
    ~SelectControl() override = default;

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

class SelectControlFactory : public RenderActionFactory {
public:
    SelectControlFactory() = default;
    ~SelectControlFactory() = default;

    [[nodiscard]] tObjectPtr create() override { return std::make_unique<SelectControl>(); }
};

} // namespace OpenGeoLab::Render
