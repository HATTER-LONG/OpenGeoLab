/**
 * @file RenderAction.hpp
 * @brief Pluggable render action contracts used by the render service.
 */

#pragma once

#include <ogl/core/IService.hpp>
#include <ogl/render/export.hpp>

#include <kangaroo/util/factorytraits.hpp>

#include <memory>

namespace OGL::Render {

/**
 * @brief Action interface executed by the render service for a specific action id.
 */
class OGL_RENDER_EXPORT RenderAction {
public:
    virtual ~RenderAction() = default;

    /**
     * @brief Execute the render action.
     * @param request Full service request including module, action, and param.
     * @param progress_callback Optional progress reporter used by downstream callers.
     * @return Structured response for the executed action.
     */
    virtual auto execute(const OGL::Core::ServiceRequest& request,
                         const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse = 0;
};

/**
 * @brief Kangaroo factory contract for creating render actions.
 */
class OGL_RENDER_EXPORT RenderActionFactory
    : public Kangaroo::Util::FactoryTraits<RenderActionFactory, RenderAction> {
public:
    virtual ~RenderActionFactory() = default;

    /**
     * @brief Create a new action instance.
     * @return Unique action pointer.
     */
    virtual auto create() -> tObjectPtr = 0;
};

} // namespace OGL::Render
