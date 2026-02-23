/**
 * @file render_service.cpp
 * @brief Implementation of RenderService for dispatching render actions
 */

#include "render/render_service.hpp"
#include "action/select_control.hpp"
#include "action/viewport_control.hpp"
#include "render/scene_renderer.hpp"


#include "render/render_action.hpp"
#include "util/logger.hpp"

#include <QMetaObject>
#include <QString>
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Render {

nlohmann::json RenderService::processRequest(const std::string& /*module_name*/,
                                             const nlohmann::json& params,
                                             App::IProgressReporterPtr /*progress_reporter*/) {
    nlohmann::json response;

    if(!params.contains("action") || !params["action"].is_string()) {
        LOG_ERROR("RenderService: Missing or invalid 'action' parameter");
        throw std::invalid_argument("Missing or invalid 'action' parameter.");
    }

    const std::string action = params["action"].get<std::string>();

    auto render_action = g_ComponentFactory.createObjectWithID<RenderActionFactory>(action);
    return render_action->execute(params, nullptr);
}

RenderServiceFactory::tObjectSharedPtr RenderServiceFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<RenderService>();
    return singleton_instance;
}

void registerServices() {
    LOG_DEBUG("RenderService: Registering render services");
    g_ComponentFactory.registInstanceFactoryWithID<RenderServiceFactory>("RenderService");
    g_ComponentFactory.registFactoryWithID<ViewPortControlFactory>("ViewPortControl");
    g_ComponentFactory.registFactoryWithID<SelectControlFactory>("SelectControl");
    registerSceneRendererFactory();
}

} // namespace OpenGeoLab::Render
