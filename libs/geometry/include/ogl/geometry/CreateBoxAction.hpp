/**
 * @file CreateBoxAction.hpp
 * @brief Geometry action that creates a box model.
 */

#pragma once

#include <ogl/geometry/GeometryAction.hpp>

namespace OGL::Geometry {

class OGL_GEOMETRY_EXPORT CreateBoxAction final : public GeometryAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "createBox"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_GEOMETRY_EXPORT CreateBoxActionFactory final : public GeometryActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<CreateBoxAction>(); }
};

} // namespace OGL::Geometry
