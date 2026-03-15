#include <ogl/scene/SceneComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/scene/BuildSceneAction.hpp>
#include <ogl/scene/SceneAction.hpp>
#include <ogl/scene/SceneLogger.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <algorithm>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace {

auto supportedSceneActions() -> std::vector<std::string> {
    return {OGL::Scene::BuildSceneAction::actionName()};
}

auto supportedSceneActionSummary() -> std::string {
    auto action_names = supportedSceneActions();
    std::sort(action_names.begin(), action_names.end());

    std::ostringstream stream;
    for(std::size_t index = 0; index < action_names.size(); ++index) {
        if(index > 0) {
            stream << ", ";
        }
        stream << action_names[index];
    }

    return stream.str();
}

auto unsupportedSceneActionResponse(const OGL::Core::ServiceRequest& request)
    -> OGL::Core::ServiceResponse {
    return {
        .success = false,
        .module = request.module,
        .action = request.action,
        .message = "Unsupported scene action. Registered actions: " +
                   supportedSceneActionSummary() + ".",
        .payload = nlohmann::json::object(),
    };
}

class SceneService final : public OGL::Core::IService {
public:
    auto processRequest(const OGL::Core::ServiceRequest& request,
                        const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override {
        if(request.module != "scene") {
            return {
                .success = false,
                .module = request.module,
                .action = request.action,
                .message = "Scene service only accepts the scene module.",
                .payload = nlohmann::json::object(),
            };
        }

        const auto action_id = request.action;
        const auto supported_actions = supportedSceneActions();
        if(std::find(supported_actions.begin(), supported_actions.end(), action_id) ==
           supported_actions.end()) {
            return unsupportedSceneActionResponse(request);
        }

        try {
            auto action = g_ComponentFactory.createObjectWithID<OGL::Scene::SceneActionFactory>(
                action_id);
            if(!action) {
                return {
                    .success = false,
                    .module = request.module,
                    .action = request.action,
                    .message = "Scene action factory resolved a null action instance.",
                    .payload = nlohmann::json::object(),
                };
            }

            OGL_SCENE_LOG_INFO("Dispatching scene action={} through pluggable action component",
                               request.action);
            return action->execute(request, progress_callback);
        } catch(const std::exception& ex) {
            OGL_SCENE_LOG_ERROR("Scene action={} failed during dispatch error={}", request.action,
                                ex.what());
            return {
                .success = false,
                .module = request.module,
                .action = request.action,
                .message = ex.what(),
                .payload = nlohmann::json::object(),
            };
        }
    }
};

class SceneServiceFactory final : public OGL::Core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<SceneService>();
        return service;
    }
};

} // namespace

namespace OGL::Scene {

void registerSceneComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        g_ComponentFactory.registInstanceFactoryWithID<SceneServiceFactory>("scene");
        g_ComponentFactory.registFactoryWithID<BuildSceneActionFactory>(
            BuildSceneAction::actionName());
        OGL_SCENE_LOG_INFO("Registered scene service '{}' with actions: {}", "scene",
                           supportedSceneActionSummary());
    });
}

} // namespace OGL::Scene
