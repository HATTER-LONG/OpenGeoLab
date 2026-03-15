#include <ogl/geometry/CreateSphereAction.hpp>

#include "GeometryActionUtilities.hpp"

namespace OGL::Geometry {

auto CreateSphereAction::execute(const OGL::Core::ServiceRequest& request,
                                 const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    const Internal::GeometryCreateSpec create_spec{
        .action = std::string{actionName()},
        .shapeType = "sphere",
        .defaultModelName = "Sphere_001",
    };

    if(!Internal::reportProgress(progress_callback, 0.2, "Normalizing sphere parameters...")) {
        return Internal::cancellationResponse(request, "Sphere creation was cancelled.");
    }

    const auto normalized_param = Internal::normalizeSphereParam(create_spec, request.param);
    const OGL::Geometry::GeometryModel model(
        Internal::buildDescriptor(normalized_param));

    OGL_GEOMETRY_LOG_INFO("Accepted geometry action={} shape={} name={} source={}", request.action,
                          create_spec.shapeType, model.modelName(), model.source());
    Internal::reportProgress(progress_callback, 0.9, "Sphere request completed.");
    return Internal::buildCreateResponse(request, create_spec, normalized_param, model);
}

} // namespace OGL::Geometry

