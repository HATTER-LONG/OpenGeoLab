#include <ogl/geometry/GeometryComponentRegistration.hpp>

#include <ogl/core/ActionServiceRegistration.hpp>
#include <ogl/geometry/CreateBoxAction.hpp>
#include <ogl/geometry/CreateCylinderAction.hpp>
#include <ogl/geometry/CreateSphereAction.hpp>
#include <ogl/geometry/CreateTorusAction.hpp>
#include <ogl/geometry/GeometryAction.hpp>
#include <ogl/geometry/GeometryLogger.hpp>
#include <ogl/geometry/InspectModelAction.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace {

auto supportedGeometryActions() -> std::vector<std::string> {
    return {OGL::Geometry::CreateBoxAction::actionName(),
            OGL::Geometry::CreateCylinderAction::actionName(),
            OGL::Geometry::CreateSphereAction::actionName(),
            OGL::Geometry::CreateTorusAction::actionName(),
            OGL::Geometry::InspectModelAction::actionName()};
}

struct GeometryLoggerHooks {
    void onRegistered(const std::string& moduleName, const std::string& sortedActions) const {
        OGL_GEOMETRY_LOG_INFO("Registered geometry service '{}' with actions: {}", moduleName,
                              sortedActions);
    }

    void onDispatch(const std::string&, const std::string& actionName) const {
        OGL_GEOMETRY_LOG_INFO("Dispatching geometry action={} through pluggable action component",
                              actionName);
    }

    void onFactoryNull(const std::string& moduleName, const std::string& actionName) const {
        OGL_GEOMETRY_LOG_ERROR("{} action factory resolved a null action instance for action={}.",
                               moduleName, actionName);
    }

    void
    onError(const std::string&, const std::string& actionName, const std::string& errorText) const {
        OGL_GEOMETRY_LOG_ERROR("Geometry action={} failed during dispatch error={}", actionName,
                               errorText);
    }
};

auto makeGeometryServiceRegistrationSpec()
    -> OGL::Core::ActionServiceRegistrationSpec<OGL::Geometry::GeometryAction,
                                                GeometryLoggerHooks> {
    return {.moduleName = "geometry",
            .supportedActions = supportedGeometryActions(),
            .createActionById =
                [](const std::string& actionId) -> std::shared_ptr<OGL::Geometry::GeometryAction> {
                return g_ComponentFactory.createObjectWithID<OGL::Geometry::GeometryActionFactory>(
                    actionId);
            },
            .loggerHooks = {}};
}

using GeometryServiceFactory =
    OGL::Core::ActionServiceFactory<OGL::Geometry::GeometryAction, GeometryLoggerHooks>;

} // namespace

namespace OGL::Geometry {

void registerGeometryComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        OGL::Core::registerActionService<GeometryServiceFactory>(
            makeGeometryServiceRegistrationSpec());
        g_ComponentFactory.registFactoryWithID<InspectModelActionFactory>(
            InspectModelAction::actionName());
        g_ComponentFactory.registFactoryWithID<CreateBoxActionFactory>(
            CreateBoxAction::actionName());
        g_ComponentFactory.registFactoryWithID<CreateCylinderActionFactory>(
            CreateCylinderAction::actionName());
        g_ComponentFactory.registFactoryWithID<CreateSphereActionFactory>(
            CreateSphereAction::actionName());
        g_ComponentFactory.registFactoryWithID<CreateTorusActionFactory>(
            CreateTorusAction::actionName());
    });
}

} // namespace OGL::Geometry
