#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <ogl/core/ActionServiceRegistration.hpp>
#include <ogl/selection/BoxSelectAction.hpp>
#include <ogl/selection/PickEntityAction.hpp>
#include <ogl/selection/SelectionAction.hpp>
#include <ogl/selection/SelectionLogger.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace {

auto supportedSelectionActions() -> std::vector<std::string> {
    return {OGL::Selection::BoxSelectAction::actionName(),
            OGL::Selection::PickEntityAction::actionName()};
}

struct SelectionLoggerHooks {
    void onRegistered(const std::string& moduleName, const std::string& sortedActions) const {
        OGL_SELECTION_LOG_INFO("Registered selection service '{}' with actions: {}", moduleName,
                               sortedActions);
    }

    void onDispatch(const std::string&, const std::string& actionName) const {
        OGL_SELECTION_LOG_INFO("Dispatching selection action={} through pluggable action component",
                               actionName);
    }

    void onFactoryNull(const std::string& moduleName, const std::string& actionName) const {
        OGL_SELECTION_LOG_ERROR("{} action factory resolved a null action instance for action={}.",
                                moduleName, actionName);
    }

    void
    onError(const std::string&, const std::string& actionName, const std::string& errorText) const {
        OGL_SELECTION_LOG_ERROR("Selection action={} failed during dispatch error={}", actionName,
                                errorText);
    }
};

auto makeSelectionServiceRegistrationSpec()
    -> OGL::Core::ActionServiceRegistrationSpec<OGL::Selection::SelectionAction,
                                                SelectionLoggerHooks> {
    return {
        .moduleName = "selection",
        .supportedActions = supportedSelectionActions(),
        .createActionById =
            [](const std::string& actionId) -> std::shared_ptr<OGL::Selection::SelectionAction> {
            return g_ComponentFactory.createObjectWithID<OGL::Selection::SelectionActionFactory>(
                actionId);
        },
        .loggerHooks = {}};
}

using SelectionServiceFactory =
    OGL::Core::ActionServiceFactory<OGL::Selection::SelectionAction, SelectionLoggerHooks>;

} // namespace

namespace OGL::Selection {

void registerSelectionComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        OGL::Core::registerActionService<SelectionServiceFactory>(
            makeSelectionServiceRegistrationSpec());
        g_ComponentFactory.registFactoryWithID<PickEntityActionFactory>(
            PickEntityAction::actionName());
        g_ComponentFactory.registFactoryWithID<BoxSelectActionFactory>(
            BoxSelectAction::actionName());
    });
}

} // namespace OGL::Selection
