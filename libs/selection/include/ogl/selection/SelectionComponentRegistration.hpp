/**
 * @file SelectionComponentRegistration.hpp
 * @brief Registers placeholder selection services into Kangaroo ComponentFactory.
 */

#pragma once

#include <ogl/selection/export.hpp>

namespace ogl::selection {

/**
 * @brief Register selection services exactly once for the current process.
 */
OGL_SELECTION_EXPORT void registerSelectionComponents();

} // namespace ogl::selection