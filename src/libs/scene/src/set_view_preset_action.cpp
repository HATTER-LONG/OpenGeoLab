/**
 * @file set_view_preset_action.cpp
 * @brief SetViewPresetAction implementation
 */

#include <opengeolab/scene/set_view_preset_action.hpp>

#include <opengeolab/scene/viewport_state.hpp>

#include <optional>
#include <string>

namespace OpenGeoLab::Scene {

namespace {

std::optional<ViewPreset> parseViewPreset(const std::string& name) {
    if(name == "Front")
        return ViewPreset::Front;
    if(name == "Back")
        return ViewPreset::Back;
    if(name == "Top")
        return ViewPreset::Top;
    if(name == "Bottom")
        return ViewPreset::Bottom;
    if(name == "Left")
        return ViewPreset::Left;
    if(name == "Right")
        return ViewPreset::Right;
    if(name == "Isometric")
        return ViewPreset::Isometric;
    return std::nullopt;
}

} // namespace

SetViewPresetAction::SetViewPresetAction(ViewportState& state) : m_state(state) {}
SetViewPresetAction::~SetViewPresetAction() = default;

nlohmann::json SetViewPresetAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Apply a standard camera view preset."},
        {"params",
         {{"preset",
           {{"type", "string"},
            {"required", true},
            {"description", "One of: Front, Back, Top, Bottom, Left, Right, Isometric"}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}}}}};
}

nlohmann::json SetViewPresetAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto preset_name = param.value("preset", std::string{});
    const auto preset = parseViewPreset(preset_name);
    if(!preset.has_value()) {
        return {
            {"ok", false}, {"action", ACTION_NAME}, {"error", "Unknown preset: " + preset_name}};
    }

    m_state.setViewPreset(*preset);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
