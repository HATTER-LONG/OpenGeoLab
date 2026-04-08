/**
 * @file set_display_mode_action.cpp
 * @brief SetDisplayModeAction implementation
 */

#include <opengeolab/scene/set_display_mode_action.hpp>

#include <opengeolab/scene/viewport_state.hpp>

namespace OpenGeoLab::Scene {

SetDisplayModeAction::SetDisplayModeAction(ViewportState& state) : m_state(state) {}
SetDisplayModeAction::~SetDisplayModeAction() = default;

nlohmann::json SetDisplayModeAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Control viewport rendering display modes (x-ray, tessellation overlay)."},
        {"params",
         {{"xRayMode",
           {{"type", "boolean"},
            {"required", false},
            {"description", "Enable semi-transparent rendering to see through surfaces."}}},
          {"showTessellation",
           {{"type", "boolean"},
            {"required", false},
            {"description",
             "Overlay tessellation triangle edges and vertices on the model."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"xRayMode",
           {{"type", "boolean"}, {"description", "Current x-ray rendering state."}}},
          {"showTessellation",
           {{"type", "boolean"},
            {"description", "Current tessellation overlay state."}}}}}};
}

nlohmann::json SetDisplayModeAction::execute(const nlohmann::json& param,
                                             const Core::ProgressCallback& progress) {
    if(param.contains("xRayMode") && param["xRayMode"].is_boolean()) {
        m_state.setXRayMode(param["xRayMode"].get<bool>());
    }
    if(param.contains("showTessellation") && param["showTessellation"].is_boolean()) {
        m_state.setShowTessellation(param["showTessellation"].get<bool>());
    }

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true},
            {"action", ACTION_NAME},
            {"xRayMode", m_state.xRayMode()},
            {"showTessellation", m_state.showTessellation()}};
}

} // namespace OpenGeoLab::Scene
