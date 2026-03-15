#include <ogl/geometry/CreateTorusAction.hpp>

#include "GeometryActionUtilities.hpp"

namespace OGL::Geometry {

auto CreateTorusAction::execute(const OGL::Core::ServiceRequest& request,
                                const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    const Internal::GeometryCreateSpec create_spec{
        .action = std::string{actionName()},
        .shapeType = "torus",
        .defaultModelName = "Torus_001",
    };

    if(!Internal::reportProgress(progress_callback, 0.2, "Normalizing torus parameters...")) {
        return Internal::cancellationResponse(request, "Torus creation was cancelled.");
    }

    const auto normalized_param = Internal::normalizeTorusParam(create_spec, request.param);
    const OGL::Geometry::GeometryModel model(
        Internal::buildDescriptor(normalized_param));

    OGL_GEOMETRY_LOG_INFO("Accepted geometry action={} shape={} name={} source={}", request.action,
                          create_spec.shapeType, model.modelName(), model.source());
    Internal::reportProgress(progress_callback, 0.9, "Torus request completed.");
    return Internal::buildCreateResponse(request, create_spec, normalized_param, model);
}

} // namespace OGL::Geometry

