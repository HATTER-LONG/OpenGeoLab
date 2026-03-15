#include <ogl/render/RenderComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/render/BuildFrameAction.hpp>
#include <ogl/render/RenderAction.hpp>
#include <ogl/render/RenderLogger.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <algorithm>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace {

auto supportedRenderActions() -> std::vector<std::string> {
    return {OGL::Render::BuildFrameAction::actionName()};
}

auto supportedRenderActionSummary() -> std::string {
    auto action_names = supportedRenderActions();
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

auto unsupportedRenderActionResponse(const OGL::Core::ServiceRequest& request)
    -> OGL::Core::ServiceResponse {
    return {
        .success = false,
        .module = request.module,
        .action = request.action,
        .message = "Unsupported render action. Registered actions: " +
                   supportedRenderActionSummary() + ".",
        .payload = nlohmann::json::object(),
    };
}

class RenderService final : public OGL::Core::IService {
public:
    auto processRequest(const OGL::Core::ServiceRequest& request,
                        const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override {
        if(request.module != "render") {
            return {
                .success = false,
                .module = request.module,
                .action = request.action,
                .message = "Render service only accepts the render module.",
                .payload = nlohmann::json::object(),
            };
        }

        const auto action_id = request.action;
        const auto supported_actions = supportedRenderActions();
        if(std::find(supported_actions.begin(), supported_actions.end(), action_id) ==
           supported_actions.end()) {
            return unsupportedRenderActionResponse(request);
        }

        try {
            auto action = g_ComponentFactory.createObjectWithID<OGL::Render::RenderActionFactory>(
                action_id);
            if(!action) {
                return {
                    .success = false,
                    .module = request.module,
                    .action = request.action,
                    .message = "Render action factory resolved a null action instance.",
                    .payload = nlohmann::json::object(),
                };
            }

            OGL_RENDER_LOG_INFO("Dispatching render action={} through pluggable action component",
                                request.action);
            return action->execute(request, progress_callback);
        } catch(const std::exception& ex) {
            OGL_RENDER_LOG_ERROR("Render action={} failed during dispatch error={}", request.action,
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

class RenderServiceFactory final : public OGL::Core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<RenderService>();
        return service;
    }
};

} // namespace

namespace OGL::Render {

void registerRenderComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        g_ComponentFactory.registInstanceFactoryWithID<RenderServiceFactory>("render");
        g_ComponentFactory.registFactoryWithID<BuildFrameActionFactory>(
            BuildFrameAction::actionName());
        OGL_RENDER_LOG_INFO("Registered render service '{}' with actions: {}", "render",
                            supportedRenderActionSummary());
    });
}

} // namespace OGL::Render
