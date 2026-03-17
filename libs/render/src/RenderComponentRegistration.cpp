#include <ogl/render/RenderComponentRegistration.hpp>

#include <ogl/core/ActionServiceRegistration.hpp>
#include <ogl/render/BuildFrameAction.hpp>
#include <ogl/render/RenderAction.hpp>
#include <ogl/render/RenderLogger.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace {

auto supportedRenderActions() -> std::vector<std::string> {
    return {OGL::Render::BuildFrameAction::actionName()};
}

struct RenderLoggerHooks {
    void onRegistered(const std::string& moduleName, const std::string& sortedActions) const {
        OGL_RENDER_LOG_INFO("Registered render service '{}' with actions: {}", moduleName,
                            sortedActions);
    }

    void onDispatch(const std::string&, const std::string& actionName) const {
        OGL_RENDER_LOG_INFO("Dispatching render action={} through pluggable action component",
                            actionName);
    }

    void onFactoryNull(const std::string& moduleName, const std::string& actionName) const {
        OGL_RENDER_LOG_ERROR("{} action factory resolved a null action instance for action={}.",
                             moduleName, actionName);
    }

    void
    onError(const std::string&, const std::string& actionName, const std::string& errorText) const {
        OGL_RENDER_LOG_ERROR("Render action={} failed during dispatch error={}", actionName,
                             errorText);
    }
};

auto makeRenderServiceRegistrationSpec()
    -> OGL::Core::ActionServiceRegistrationSpec<OGL::Render::RenderAction, RenderLoggerHooks> {
    return {.moduleName = "render",
            .supportedActions = supportedRenderActions(),
            .createActionById =
                [](const std::string& actionId) -> std::shared_ptr<OGL::Render::RenderAction> {
                return g_ComponentFactory.createObjectWithID<OGL::Render::RenderActionFactory>(
                    actionId);
            },
            .loggerHooks = {}};
}

using RenderServiceFactory =
    OGL::Core::ActionServiceFactory<OGL::Render::RenderAction, RenderLoggerHooks>;

} // namespace

namespace OGL::Render {

void registerRenderComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        OGL::Core::registerActionService<RenderServiceFactory>(makeRenderServiceRegistrationSpec());
        g_ComponentFactory.registFactoryWithID<BuildFrameActionFactory>(
            BuildFrameAction::actionName());
    });
}

} // namespace OGL::Render
