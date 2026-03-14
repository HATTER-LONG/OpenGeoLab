/**
 * @file RenderComponentRegistration.hpp
 * @brief Registers placeholder render services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/render/export.hpp>

namespace OGL::Render {

/**
 * @brief Register render services exactly once for the current process.
 */
OGL_RENDER_EXPORT void registerRenderComponents();

} // namespace OGL::Render