/**
 * @file viewport_control.cpp
 * @brief Implementation of ViewPortControl render action
 */

#include "viewport_control.hpp"
#include "render/render_scene_controller.hpp"

namespace OpenGeoLab::Render {
nlohmann::json ViewPortControl::execute(const nlohmann::json& params,
                                        Util::ProgressCallback /*progress_callback*/) {
    // Extract action parameter
    if(!params.is_object() || !params.contains("view_ctrl") || !params["view_ctrl"].is_object()) {
        throw std::runtime_error("Missing or invalid 'view_ctrl' parameter.");
    }

    const auto& view_ctrl = params["view_ctrl"];
    if(view_ctrl.contains("view") && view_ctrl["view"].is_number_integer()) {
        const ViewPreset view = static_cast<ViewPreset>(view_ctrl["view"].get<int>());
        applyPreset(view);
    } else if(view_ctrl.contains("refresh") && view_ctrl["refresh"].is_boolean() &&
              view_ctrl["refresh"].get<bool>()) {
        RenderSceneController::instance().refreshScene();
    } else if(view_ctrl.contains("fit") && view_ctrl["fit"].is_boolean() &&
              view_ctrl["fit"].get<bool>()) {
        RenderSceneController::instance().fitToScene();
    } else {
        throw std::runtime_error("Unsupported or missing view control action.");
    }
    return nlohmann::json{{"status", "success"}, {"action", params["action"]}};
}

void ViewPortControl::applyPreset(ViewPreset preset) {
    auto& render_service = RenderSceneController::instance();

    switch(preset) {
    case ViewPreset::Front:
        render_service.setFrontView();
        break;
    case ViewPreset::Back:
        render_service.setBackView();
        break;
    case ViewPreset::Left:
        render_service.setLeftView();
        break;
    case ViewPreset::Right:
        render_service.setRightView();
        break;
    case ViewPreset::Top:
        render_service.setTopView();
        break;
    case ViewPreset::Bottom:
        render_service.setBottomView();
        break;
    default:
        throw std::runtime_error("Unsupported view preset.");
    }
}
} // namespace OpenGeoLab::Render