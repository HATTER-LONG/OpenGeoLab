/**
 * @file PickEntityAction.hpp
 * @brief Selection action that resolves a single pick hit from scene and render data.
 */

#pragma once

#include <ogl/selection/SelectionAction.hpp>

namespace OGL::Selection {

class OGL_SELECTION_EXPORT PickEntityAction final : public SelectionAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "pickEntity"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_SELECTION_EXPORT PickEntityActionFactory final : public SelectionActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<PickEntityAction>(); }
};

} // namespace OGL::Selection
