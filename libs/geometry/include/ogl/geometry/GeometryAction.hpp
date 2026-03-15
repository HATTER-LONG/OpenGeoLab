/**
 * @file GeometryAction.hpp
 * @brief Pluggable geometry action contracts used by the geometry service.
 */

#pragma once

#include <ogl/core/IService.hpp>
#include <ogl/geometry/export.hpp>

#include <kangaroo/util/factorytraits.hpp>

#include <memory>

namespace OGL::Geometry {

/**
 * @brief Action interface executed by the geometry service for a specific action id.
 */
class OGL_GEOMETRY_EXPORT GeometryAction {
public:
    virtual ~GeometryAction() = default;

    /**
     * @brief Execute the geometry action.
     * @param request Full service request including module, action, and param.
     * @param progress_callback Optional progress reporter used by the UI overlay.
     * @return Structured response for the executed action.
     */
    virtual auto execute(const OGL::Core::ServiceRequest& request,
                         const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse = 0;
};

/**
 * @brief Kangaroo factory contract for creating geometry actions.
 */
class OGL_GEOMETRY_EXPORT GeometryActionFactory
    : public Kangaroo::Util::FactoryTraits<GeometryActionFactory, GeometryAction> {
public:
    virtual ~GeometryActionFactory() = default;

    /**
     * @brief Create a new action instance.
     * @return Unique action pointer.
     */
    virtual auto create() -> tObjectPtr = 0;
};

} // namespace OGL::Geometry
