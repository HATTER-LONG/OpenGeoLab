/**
 * @file SelectionAction.hpp
 * @brief Pluggable selection action contracts used by the selection service.
 */

#pragma once

#include <ogl/core/IService.hpp>
#include <ogl/selection/export.hpp>

#include <kangaroo/util/factorytraits.hpp>

#include <memory>

namespace OGL::Selection {

/**
 * @brief Action interface executed by the selection service for a specific action id.
 */
class OGL_SELECTION_EXPORT SelectionAction {
public:
    virtual ~SelectionAction() = default;

    /**
     * @brief Execute the selection action.
     * @param request Full service request including module, action, and param.
     * @param progress_callback Optional progress reporter used by downstream callers.
     * @return Structured response for the executed action.
     */
    virtual auto execute(const OGL::Core::ServiceRequest& request,
                         const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse = 0;
};

/**
 * @brief Kangaroo factory contract for creating selection actions.
 */
class OGL_SELECTION_EXPORT SelectionActionFactory
    : public Kangaroo::Util::FactoryTraits<SelectionActionFactory, SelectionAction> {
public:
    virtual ~SelectionActionFactory() = default;

    /**
     * @brief Create a new action instance.
     * @return Unique action pointer.
     */
    virtual auto create() -> tObjectPtr = 0;
};

} // namespace OGL::Selection
