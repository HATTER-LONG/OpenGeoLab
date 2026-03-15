#include <ogl/selection/BoxSelectAction.hpp>

#include "SelectionActionUtilities.hpp"

namespace OGL::Selection {

auto BoxSelectAction::execute(const OGL::Core::ServiceRequest& request,
                              const OGL::Core::ProgressCallback& progress_callback)
    -> OGL::Core::ServiceResponse {
    return Internal::buildSelectionResponse(request, actionName(),
                                            Internal::normalizeBoxSelectParam(request.param),
                                            progress_callback);
}

} // namespace OGL::Selection
