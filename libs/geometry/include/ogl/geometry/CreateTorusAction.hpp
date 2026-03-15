/**
 * @file CreateTorusAction.hpp
 * @brief Geometry action that creates a torus model.
 */

#pragma once

#include <ogl/geometry/GeometryAction.hpp>

namespace OGL::Geometry {

class OGL_GEOMETRY_EXPORT CreateTorusAction final : public GeometryAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "createTorus"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_GEOMETRY_EXPORT CreateTorusActionFactory final : public GeometryActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<CreateTorusAction>(); }
};

} // namespace OGL::Geometry
