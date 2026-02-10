/**
 * @file mesh_service.cpp
 * @brief Implementation of MeshService request processing
 */

#include "mesh/mesh_service.hpp"

#include "mesh/mesh_action.hpp"
#include "mesh_documentImpl.hpp"
#include "render/render_scene_controller.hpp"
#include "util/logger.hpp"
#include "util/progress_bridge.hpp"

#include "action/generate_mesh_action.hpp"

#include <QCoreApplication>
#include <QMetaObject>
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::Mesh {

nlohmann::json MeshService::processRequest(const std::string& module_name,
                                           const nlohmann::json& params,
                                           App::IProgressReporterPtr progress_reporter) {
    LOG_DEBUG("MeshService: Processing request for module: {}", module_name);

    if(!params.contains("action") || !params["action"].is_string()) {
        LOG_ERROR("MeshService: Missing or invalid 'action' parameter");
        throw std::invalid_argument("Missing or invalid 'action' parameter.");
    }

    const std::string action_name = params["action"].get<std::string>();
    LOG_TRACE("MeshService: Executing action '{}'", action_name);

    auto action = g_ComponentFactory.createObjectWithID<MeshActionFactory>(action_name);

    nlohmann::json result =
        action->execute(params, Util::makeProgressCallback(progress_reporter, 0.0, 1.0));

    if(!result.value("success", false)) {
        const std::string error_msg = result.value("error", "Unknown error");
        LOG_ERROR("MeshService: Action '{}' failed: {}", action_name, error_msg);
        throw std::runtime_error("Mesh action '" + action_name + "' failed: " + error_msg);
    }

    LOG_TRACE("MeshService: Action '{}' completed successfully", action_name);

    // Trigger scene refresh on the main thread so the new mesh data gets rendered
    if(auto* app = QCoreApplication::instance()) {
        QMetaObject::invokeMethod(
            app, []() { Render::RenderSceneController::instance().refreshScene(true); },
            Qt::QueuedConnection);
    }

    return result;
}

MeshServiceFactory::tObjectSharedPtr MeshServiceFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<MeshService>();
    return singleton_instance;
}

void registerServices() {
    LOG_DEBUG("MeshService: Registering mesh services and actions");
    g_ComponentFactory.registInstanceFactoryWithID<MeshServiceFactory>("MeshService");
    g_ComponentFactory.registInstanceFactory<MeshDocumentImplSingletonFactory>();
    g_ComponentFactory.registFactoryWithID<GenerateMeshActionFactory>(
        GenerateMeshAction::actionName());
}

} // namespace OpenGeoLab::Mesh
