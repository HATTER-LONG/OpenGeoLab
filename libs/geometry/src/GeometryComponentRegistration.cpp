#include <ogl/geometry/GeometryComponentRegistration.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/CreateBoxAction.hpp>
#include <ogl/geometry/CreateCylinderAction.hpp>
#include <ogl/geometry/CreateSphereAction.hpp>
#include <ogl/geometry/CreateTorusAction.hpp>
#include <ogl/geometry/GeometryAction.hpp>
#include <ogl/geometry/GeometryLogger.hpp>
#include <ogl/geometry/InspectModelAction.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <algorithm>
#include <mutex>
#include <sstream>
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

auto supportedGeometryActionSummary() -> std::string {
    auto action_names = supportedGeometryActions();
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

auto unsupportedGeometryActionResponse(const OGL::Core::ServiceRequest& request)
    -> OGL::Core::ServiceResponse {
    return {
        .success = false,
        .module = request.module,
        .action = request.action,
        .message = "Unsupported geometry action. Registered actions: " +
                   supportedGeometryActionSummary() + ".",
        .payload = nlohmann::json::object(),
    };
}

class GeometryService final : public OGL::Core::IService {
public:
    auto processRequest(const OGL::Core::ServiceRequest& request,
                        const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override {
        if(request.module != "geometry") {
            return {
                .success = false,
                .module = request.module,
                .action = request.action,
                .message = "Geometry service only accepts the geometry module.",
                .payload = nlohmann::json::object(),
            };
        }

        const auto action_id = request.action;
        const auto supported_actions = supportedGeometryActions();
        if(std::find(supported_actions.begin(), supported_actions.end(), action_id) ==
           supported_actions.end()) {
            return unsupportedGeometryActionResponse(request);
        }

        try {
            auto action =
                g_ComponentFactory.createObjectWithID<OGL::Geometry::GeometryActionFactory>(
                    action_id);
            if(!action) {
                return {
                    .success = false,
                    .module = request.module,
                    .action = request.action,
                    .message = "Geometry action factory resolved a null action instance.",
                    .payload = nlohmann::json::object(),
                };
            }

            OGL_GEOMETRY_LOG_INFO(
                "Dispatching geometry action={} through pluggable action component",
                request.action);
            return action->execute(request, progress_callback);
        } catch(const std::exception& ex) {
            OGL_GEOMETRY_LOG_ERROR("Geometry action={} failed during dispatch error={}",
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

class GeometryServiceFactory final : public OGL::Core::IServiceSingletonFactory {
public:
    auto instance() const -> tObjectSharedPtr override {
        static auto service = std::make_shared<GeometryService>();
        return service;
    }
};

} // namespace

namespace OGL::Geometry {

void registerGeometryComponents() {
    static std::once_flag once;
    std::call_once(once, []() {
        g_ComponentFactory.registInstanceFactoryWithID<GeometryServiceFactory>("geometry");
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

        OGL_GEOMETRY_LOG_INFO("Registered geometry service '{}' with actions: {}", "geometry",
                              supportedGeometryActionSummary());
    });
}

} // namespace OGL::Geometry
