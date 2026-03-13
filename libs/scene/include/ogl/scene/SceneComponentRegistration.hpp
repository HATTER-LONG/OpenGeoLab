/**
 * @file SceneComponentRegistration.hpp
 * @brief Registers placeholder scene services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/scene/export.hpp>

namespace ogl::scene {

/**
 * @brief Register scene services exactly once for the current process.
 */
OGL_SCENE_EXPORT void registerSceneComponents();

} // namespace ogl::scene