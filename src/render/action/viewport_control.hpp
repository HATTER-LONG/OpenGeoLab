#pragma once

#include "render/render_action.hpp"

namespace OpenGeoLab::Render {

enum class ViewPreset { Front = 0, Back = 1, Left = 2, Right = 3, Top = 4, Bottom = 5 };

class ViewPortControl : public RenderActionBase {
public:
    ViewPortControl() = default;
    ~ViewPortControl() override = default;

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;

private:
    void applyPreset(ViewPreset preset);

private:
    ViewPreset m_preset{ViewPreset::Front};
};

class ViewPortControlFactory : public RenderActionFactory {
public:
    ViewPortControlFactory() = default;
    ~ViewPortControlFactory() = default;

    [[nodiscard]] tObjectPtr create() override { return std::make_unique<ViewPortControl>(); }
};
} // namespace OpenGeoLab::Render