/**
 * @file part_visibility_control.hpp
 * @brief Render action for per-part geometry and mesh visibility toggles.
 */

#pragma once

#include "render/render_action.hpp"

namespace OpenGeoLab::Render {

class PartVisibilityControl final : public RenderActionBase {
public:
    PartVisibilityControl() = default;
    ~PartVisibilityControl() override = default;

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

class PartVisibilityControlFactory final : public RenderActionFactory {
public:
    PartVisibilityControlFactory() = default;
    ~PartVisibilityControlFactory() = default;

    [[nodiscard]] tObjectPtr create() override { return std::make_unique<PartVisibilityControl>(); }
};

} // namespace OpenGeoLab::Render