#include <ogl/scene/SceneComponentRegistration.hpp>

#include <ogl/core/ActionServiceRegistration.hpp>
#include <ogl/scene/BuildSceneAction.hpp>
#include <ogl/scene/SceneAction.hpp>
#include <ogl/scene/SceneLogger.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace {

auto supportedSceneActions() -> std::vector<std::string> {
    return {OGL::Scene::BuildSceneAction::actionName()};
}

struct SceneLoggerHooks {
    void onRegistered(const std::string& moduleName, const std::string& sortedActions) const {
        OGL_SCENE_LOG_INFO("Registered scene service '{}' with actions: {}", moduleName,
                           sortedActions);
    }

    void onDispatch(const std::string&, const std::string& actionName) const {
        OGL_SCENE_LOG_INFO("Dispatching scene action={} through pluggable action component",
                           actionName);
    }

    void onFactoryNull(const std::string& moduleName, const std::string& actionName) const {
        OGL_SCENE_LOG_ERROR("{} action factory resolved a null action instance for action={}.",
                            moduleName, actionName);
    }

    void
    onError(const std::string&, const std::string& actionName, const std::string& errorText) const {
        OGL_SCENE_LOG_ERROR("Scene action={} failed during dispatch error={}", actionName,
                            errorText);
    }
};

auto makeSceneServiceRegistrationSpec()
    -> OGL::Core::ActionServiceRegistrationSpec<OGL::Scene::SceneAction, SceneLoggerHooks> {
    return {.moduleName = "scene",
            .supportedActions = supportedSceneActions(),
            .createActionById =
                [](const std::string& actionId) -> std::shared_ptr<OGL::Scene::SceneAction> {
                return g_ComponentFactory.createObjectWithID<OGL::Scene::SceneActionFactory>(
                    actionId);
            },
            .loggerHooks = {}};
}

using SceneServiceFactory =
    OGL::Core::ActionServiceFactory<OGL::Scene::SceneAction, SceneLoggerHooks>;

} // namespace

namespace OGL::Scene {

void registerSceneComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        OGL::Core::registerActionService<SceneServiceFactory>(makeSceneServiceRegistrationSpec());
        g_ComponentFactory.registFactoryWithID<BuildSceneActionFactory>(
            BuildSceneAction::actionName());
    });
}

} // namespace OGL::Scene
