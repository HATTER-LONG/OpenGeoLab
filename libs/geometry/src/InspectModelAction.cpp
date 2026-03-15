#include <ogl/geometry/InspectModelAction.hpp>

#include "GeometryActionUtilities.hpp"

namespace OGL::Geometry {

auto InspectModelAction::execute(const OGL::Core::ServiceRequest& request,
                                 const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    if(!Internal::reportProgress(progress_callback, 0.2, "Inspecting geometry model...")) {
        return Internal::cancellationResponse(request, "Geometry inspect was cancelled.");
    }

    const auto normalized_param = Internal::normalizeInspectParam(request.param);
    const OGL::Geometry::GeometryModel model(Internal::buildDescriptor(normalized_param));

    OGL_GEOMETRY_LOG_INFO("Built geometry model name={} bodies={} source={}", model.modelName(),
                          model.bodyCount(), model.source());

    Internal::reportProgress(progress_callback, 0.95, "Geometry model inspection completed.");
    return Internal::buildInspectResponse(request, normalized_param, model);
}

} // namespace OGL::Geometry
