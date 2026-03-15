/**
 * @file CreateSphereAction.hpp
 * @brief Geometry action that creates a sphere model.
 */

#pragma once

#include <ogl/geometry/GeometryAction.hpp>

namespace OGL::Geometry {

class OGL_GEOMETRY_EXPORT CreateSphereAction final : public GeometryAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "createSphere"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_GEOMETRY_EXPORT CreateSphereActionFactory final : public GeometryActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<CreateSphereAction>(); }
};

} // namespace OGL::Geometry
