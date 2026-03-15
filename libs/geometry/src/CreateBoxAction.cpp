#include <ogl/geometry/CreateBoxAction.hpp>

#include "GeometryActionUtilities.hpp"

#include <chrono>
#include <thread>

namespace OGL::Geometry {

namespace {

constexpr auto K_CREATE_BOX_STEP_DELAY = std::chrono::milliseconds(80);

auto sleepWithProgress(const OGL::Core::ServiceRequest& request,
                       const OGL::Core::ProgressCallback& progress_callback,
                       double progress,
                       const std::string& message) -> OGL::Core::ServiceResponse {
    std::this_thread::sleep_for(K_CREATE_BOX_STEP_DELAY);
    if(!Internal::reportProgress(progress_callback, progress, message)) {
        return Internal::cancellationResponse(request, "Box creation was cancelled.");
    }
    return {.success = true};
}

} // namespace

auto CreateBoxAction::execute(const OGL::Core::ServiceRequest& request,
                              const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    OGL_GEOMETRY_LOG_INFO("Starting createBox action request={}", request.toJson().dump());
    if(!Internal::reportProgress(progress_callback, 0.1, "Validating box parameters...")) {
        return Internal::cancellationResponse(request, "Box creation was cancelled.");
    }

    const Internal::GeometryCreateSpec create_spec{
        .action = std::string{actionName()},
        .shapeType = "box",
        .defaultModelName = "Box_001",
    };
    const auto normalized_param = Internal::normalizeBoxParam(create_spec, request.param);
    const OGL::Geometry::GeometryModel model(Internal::buildDescriptor(normalized_param));

    OGL_GEOMETRY_LOG_INFO(
        "Normalized createBox request name={} origin=({}, {}, {}) dimensions=({}, {}, {})",
        model.modelName(), normalized_param["origin"].value("x", 0.0),
        normalized_param["origin"].value("y", 0.0), normalized_param["origin"].value("z", 0.0),
        normalized_param["dimensions"].value("x", 0.0),
        normalized_param["dimensions"].value("y", 0.0),
        normalized_param["dimensions"].value("z", 0.0));
    OGL_GEOMETRY_LOG_WARN(
        "createBox is using simulated geometry generation for UI and logging validation");

    if(const auto response =
           sleepWithProgress(request, progress_callback, 0.3, "Preparing box profile...");
       !response.success) {
        return response;
    }
    OGL_GEOMETRY_LOG_INFO("createBox phase=prepare profile ready for {}", model.modelName());

    if(const auto response =
           sleepWithProgress(request, progress_callback, 0.55, "Building box volume...");
       !response.success) {
        return response;
    }
    OGL_GEOMETRY_LOG_INFO("createBox phase=volume volume ready for {}", model.modelName());

    if(const auto response =
           sleepWithProgress(request, progress_callback, 0.8, "Finalizing box metadata...");
       !response.success) {
        return response;
    }
    OGL_GEOMETRY_LOG_INFO("createBox phase=finalize final metadata ready for {}",
                          model.modelName());

    Internal::reportProgress(progress_callback, 0.95, "Box request completed.");
    OGL_GEOMETRY_LOG_INFO("Accepted geometry action={} shape={} name={} source={}", request.action,
                          create_spec.shapeType, model.modelName(), model.source());
    return Internal::buildCreateResponse(request, create_spec, normalized_param, model);
}

} // namespace OGL::Geometry
