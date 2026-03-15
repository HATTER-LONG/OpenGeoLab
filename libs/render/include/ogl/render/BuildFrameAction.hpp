/**
 * @file BuildFrameAction.hpp
 * @brief Render action that assembles render-frame data from a scene graph.
 */

#pragma once

#include <ogl/render/RenderAction.hpp>

namespace OGL::Render {

class OGL_RENDER_EXPORT BuildFrameAction final : public RenderAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "buildFrame"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_RENDER_EXPORT BuildFrameActionFactory final : public RenderActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<BuildFrameAction>(); }
};

} // namespace OGL::Render
