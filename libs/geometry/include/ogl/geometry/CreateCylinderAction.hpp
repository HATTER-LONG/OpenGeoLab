/**
 * @file CreateCylinderAction.hpp
 * @brief Geometry action that creates a cylinder model.
 */

#pragma once

#include <ogl/geometry/GeometryAction.hpp>

namespace OGL::Geometry {

class OGL_GEOMETRY_EXPORT CreateCylinderAction final : public GeometryAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "createCylinder"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_GEOMETRY_EXPORT CreateCylinderActionFactory final : public GeometryActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<CreateCylinderAction>(); }
};

} // namespace OGL::Geometry
