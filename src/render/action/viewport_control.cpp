/**
 * @file viewport_control.cpp
 * @brief Implementation of ViewPortControl render action
 */

#include "viewport_control.hpp"
#include "render/render_scene_controller.hpp"

namespace OpenGeoLab::Render {

namespace {

[[nodiscard]] nlohmann::json makeErrorResponse(const std::string& error) {
    return nlohmann::json{{"success", false}, {"error", error}};
}

[[nodiscard]] std::optional<RenderDisplayModeMask>
parseMeshDisplayMode(const nlohmann::json& mesh_display, std::string& error) {
    if(!mesh_display.is_object()) {
        error = "'view_ctrl.mesh_display' must be an object";
        return std::nullopt;
    }

    const bool surface = mesh_display.value("surface", false);
    const bool wireframe = mesh_display.value("wireframe", false);
    const bool points = mesh_display.value("points", false);

    RenderDisplayModeMask mode = RenderDisplayModeMask::None;
    if(surface) {
        mode = mode | RenderDisplayModeMask::Surface;
    }
    if(wireframe) {
        mode = mode | RenderDisplayModeMask::Wireframe;
    }
    if(points) {
        mode = mode | RenderDisplayModeMask::Points;
    }

    if(mode == RenderDisplayModeMask::None) {
        error = "Mesh display mode cannot be empty";
        return std::nullopt;
    }

    return mode;
}

} // namespace

nlohmann::json ViewPortControl::execute(const nlohmann::json& params,
                                        Util::ProgressCallback /*progress_callback*/) {
    if(!params.is_object() || !params.contains("view_ctrl") || !params["view_ctrl"].is_object()) {
        return makeErrorResponse("Missing or invalid 'view_ctrl' parameter");
    }

    const auto& view_ctrl = params["view_ctrl"];
    bool handled = false;

    if(view_ctrl.contains("view")) {
        if(!view_ctrl["view"].is_number_integer()) {
            return makeErrorResponse("'view_ctrl.view' must be an integer preset value");
        }

        std::string error;
        const ViewPreset view = static_cast<ViewPreset>(view_ctrl["view"].get<int>());
        if(!applyPreset(view, error)) {
            return makeErrorResponse(error);
        }
        handled = true;
    }

    if(view_ctrl.contains("refresh")) {
        if(!view_ctrl["refresh"].is_boolean()) {
            return makeErrorResponse("'view_ctrl.refresh' must be a boolean");
        }
        if(!view_ctrl["refresh"].get<bool>()) {
            return makeErrorResponse("Unsupported or missing view control action");
        }
        RenderSceneController::instance().refreshScene();
        handled = true;
    }

    if(view_ctrl.contains("mesh_display")) {
        std::string error;
        const auto mode = parseMeshDisplayMode(view_ctrl["mesh_display"], error);
        if(!mode.has_value()) {
            return makeErrorResponse(error);
        }
        RenderSceneController::instance().setMeshDisplayMode(mode.value());
        handled = true;
    }

    if(view_ctrl.contains("fit")) {
        if(!view_ctrl["fit"].is_boolean()) {
            return makeErrorResponse("'view_ctrl.fit' must be a boolean");
        }
        if(!view_ctrl["fit"].get<bool>()) {
            return makeErrorResponse("Unsupported or missing view control action");
        }
        RenderSceneController::instance().fitToScene();
        handled = true;
    }

    if(view_ctrl.contains("toggle_xray")) {
        if(!view_ctrl["toggle_xray"].is_boolean()) {
            return makeErrorResponse("'view_ctrl.toggle_xray' must be a boolean");
        }
        if(!view_ctrl["toggle_xray"].get<bool>()) {
            return makeErrorResponse("Unsupported or missing view control action");
        }
        RenderSceneController::instance().toggleXRayMode();
        handled = true;
    }

    if(view_ctrl.contains("cycle_mesh_display")) {
        if(!view_ctrl["cycle_mesh_display"].is_boolean()) {
            return makeErrorResponse("'view_ctrl.cycle_mesh_display' must be a boolean");
        }
        if(!view_ctrl["cycle_mesh_display"].get<bool>()) {
            return makeErrorResponse("Unsupported or missing view control action");
        }
        RenderSceneController::instance().cycleMeshDisplayMode();
        handled = true;
    }

    if(!handled) {
        return makeErrorResponse("Unsupported or missing view control action");
    }

    return nlohmann::json{{"success", true},
                          {"action", params.value("action", std::string("ViewPortControl"))}};
}

bool ViewPortControl::applyPreset(ViewPreset preset, std::string& error) {
    auto& render_service = RenderSceneController::instance();

    switch(preset) {
    case ViewPreset::Front:
        render_service.setFrontView();
        return true;
    case ViewPreset::Back:
        render_service.setBackView();
        return true;
    case ViewPreset::Left:
        render_service.setLeftView();
        return true;
    case ViewPreset::Right:
        render_service.setRightView();
        return true;
    case ViewPreset::Top:
        render_service.setTopView();
        return true;
    case ViewPreset::Bottom:
        render_service.setBottomView();
        return true;
    default:
        error = "Unsupported view preset";
        return false;
    }
}
} // namespace OpenGeoLab::Render