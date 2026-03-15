/**
 * @file SceneAction.hpp
 * @brief Pluggable scene action contracts used by the scene service.
 */

#pragma once

#include <ogl/core/IService.hpp>
#include <ogl/scene/export.hpp>

#include <kangaroo/util/factorytraits.hpp>

#include <memory>

namespace OGL::Scene {

/**
 * @brief Action interface executed by the scene service for a specific action id.
 */
class OGL_SCENE_EXPORT SceneAction {
public:
    virtual ~SceneAction() = default;

    /**
     * @brief Execute the scene action.
     * @param request Full service request including module, action, and param.
     * @param progress_callback Optional progress reporter used by downstream callers.
     * @return Structured response for the executed action.
     */
    virtual auto execute(const OGL::Core::ServiceRequest& request,
                         const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse = 0;
};

/**
 * @brief Kangaroo factory contract for creating scene actions.
 */
class OGL_SCENE_EXPORT SceneActionFactory
    : public Kangaroo::Util::FactoryTraits<SceneActionFactory, SceneAction> {
public:
    virtual ~SceneActionFactory() = default;

    /**
     * @brief Create a new action instance.
     * @return Unique action pointer.
     */
    virtual auto create() -> tObjectPtr = 0;
};

} // namespace OGL::Scene
