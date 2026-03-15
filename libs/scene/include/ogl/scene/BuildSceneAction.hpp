/**
 * @file BuildSceneAction.hpp
 * @brief Scene action that builds a scene graph from geometry-layer data.
 */

#pragma once

#include <ogl/scene/SceneAction.hpp>

namespace OGL::Scene {

class OGL_SCENE_EXPORT BuildSceneAction final : public SceneAction {
public:
    [[nodiscard]] static auto actionName() -> const char* { return "buildScene"; }

    auto execute(const OGL::Core::ServiceRequest& request,
                 const OGL::Core::ProgressCallback& progress_callback)
        -> OGL::Core::ServiceResponse override;
};

class OGL_SCENE_EXPORT BuildSceneActionFactory final : public SceneActionFactory {
public:
    auto create() -> tObjectPtr override { return std::make_unique<BuildSceneAction>(); }
};

} // namespace OGL::Scene
