/**
 * @file SceneComponentRegistration.hpp
 * @brief Registers scene services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/scene/export.hpp>

namespace OGL::Scene {

/**
 * @brief Register scene services exactly once for the current process.
 */
OGL_SCENE_EXPORT void registerSceneComponents();

} // namespace OGL::Scene
