/**
 * @file InspectModelAction.hpp
 * @brief Geometry action that returns geometry model information.
 */

#pragma once

#include <ogl/geometry/GeometryAction.hpp>

namespace OGL::Geometry {

class OGL_GEOMETRY_EXPORT InspectModelAction final : public GeometryAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "inspectModel"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_GEOMETRY_EXPORT InspectModelActionFactory final : public GeometryActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<InspectModelAction>(); }
};

} // namespace OGL::Geometry
