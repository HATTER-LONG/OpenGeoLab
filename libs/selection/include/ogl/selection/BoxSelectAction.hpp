/**
 * @file BoxSelectAction.hpp
 * @brief Selection action that resolves a box-selection hit set from scene and render data.
 */

#pragma once

#include <ogl/selection/SelectionAction.hpp>

namespace OGL::Selection {

class OGL_SELECTION_EXPORT BoxSelectAction final : public SelectionAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "boxSelect"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_SELECTION_EXPORT BoxSelectActionFactory final : public SelectionActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<BoxSelectAction>(); }
};

} // namespace OGL::Selection
