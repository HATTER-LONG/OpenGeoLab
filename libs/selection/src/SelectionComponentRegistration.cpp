#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/selection/BoxSelectAction.hpp>
#include <ogl/selection/PickEntityAction.hpp>
#include <ogl/selection/SelectionAction.hpp>
#include <ogl/selection/SelectionLogger.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <algorithm>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace {

auto supportedSelectionActions() -> std::vector<std::string> {
    return {OGL::Selection::BoxSelectAction::actionName(),
            OGL::Selection::PickEntityAction::actionName()};
}

auto supportedSelectionActionSummary() -> std::string {
    auto action_names = supportedSelectionActions();
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

auto unsupportedSelectionActionResponse(const OGL::Core::ServiceRequest& request)
    -> OGL::Core::ServiceResponse {
    return {
        .success = false,
        .module = request.module,
        .action = request.action,
        .message = "Unsupported selection action. Registered actions: " +
                   supportedSelectionActionSummary() + ".",
        .payload = nlohmann::json::object(),
    };
}

class SelectionService final : public OGL::Core::IService {
public:
    auto processRequest(const OGL::Core::ServiceRequest& request,
                        const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override {
        if(request.module != "selection") {
            return {
                .success = false,
                .module = request.module,
                .action = request.action,
                .message = "Selection service only accepts the selection module.",
                .payload = nlohmann::json::object(),
            };
        }

        const auto action_id = request.action;
        const auto supported_actions = supportedSelectionActions();
        if(std::find(supported_actions.begin(), supported_actions.end(), action_id) ==
           supported_actions.end()) {
            return unsupportedSelectionActionResponse(request);
        }

        try {
            auto action =
                g_ComponentFactory.createObjectWithID<OGL::Selection::SelectionActionFactory>(
                    action_id);
            if(!action) {
                return {
                    .success = false,
                    .module = request.module,
                    .action = request.action,
                    .message = "Selection action factory resolved a null action instance.",
                    .payload = nlohmann::json::object(),
                };
            }

            OGL_SELECTION_LOG_INFO(
                "Dispatching selection action={} through pluggable action component",
                request.action);
            return action->execute(request, progress_callback);
        } catch(const std::exception& ex) {
            OGL_SELECTION_LOG_ERROR("Selection action={} failed during dispatch error={}",
                                    request.action, ex.what());
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

class SelectionServiceFactory final : public OGL::Core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<SelectionService>();
        return service;
    }
};

} // namespace

namespace OGL::Selection {

void registerSelectionComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        g_ComponentFactory.registInstanceFactoryWithID<SelectionServiceFactory>("selection");
        g_ComponentFactory.registFactoryWithID<PickEntityActionFactory>(
            PickEntityAction::actionName());
        g_ComponentFactory.registFactoryWithID<BoxSelectActionFactory>(
            BoxSelectAction::actionName());
        OGL_SELECTION_LOG_INFO("Registered selection service '{}' with actions: {}", "selection",
                               supportedSelectionActionSummary());
    });
}

} // namespace OGL::Selection
