#include <ogl/geometry/CreateCylinderAction.hpp>

#include "GeometryActionUtilities.hpp"

namespace OGL::Geometry {

auto CreateCylinderAction::execute(const OGL::Core::ServiceRequest& request,
                                   const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    const Internal::GeometryCreateSpec create_spec{
        .action = std::string{actionName()},
        .shapeType = "cylinder",
        .defaultModelName = "Cylinder_001",
    };

    if(!Internal::reportProgress(progress_callback, 0.2, "Normalizing cylinder parameters...")) {
        return Internal::cancellationResponse(request, "Cylinder creation was cancelled.");
    }

    const auto normalized_param = Internal::normalizeCylinderParam(create_spec, request.param);
    const OGL::Geometry::GeometryModel model(
        Internal::buildDescriptor(normalized_param));

    OGL_GEOMETRY_LOG_INFO("Accepted geometry action={} shape={} name={} source={}", request.action,
                          create_spec.shapeType, model.modelName(), model.source());
    Internal::reportProgress(progress_callback, 0.9, "Cylinder request completed.");
    return Internal::buildCreateResponse(request, create_spec, normalized_param, model);
}

} // namespace OGL::Geometry

