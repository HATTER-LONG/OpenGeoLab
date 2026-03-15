#include <ogl/selection/PickEntityAction.hpp>

#include "SelectionActionUtilities.hpp"

namespace OGL::Selection {

auto PickEntityAction::execute(const OGL::Core::ServiceRequest& request,
                               const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    return Internal::buildSelectionResponse(request, actionName(),
                                            Internal::normalizePickParam(request.param),
                                            progress_callback);
}

} // namespace OGL::Selection
