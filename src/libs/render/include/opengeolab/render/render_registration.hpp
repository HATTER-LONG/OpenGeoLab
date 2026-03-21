/**
 * @file render_registration.hpp
 * @brief Declares the explicit registration entry point for the render module.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Render {

/**
 * @brief Registers render module services and actions with the given factory.
 *
 * Registers RenderModule as a singleton, plus all render actions
 * (camera.get_state, camera.set_state, camera.view_all, scene.add_box,
 * scene.describe) as transient entries with SceneManager injection.
 *
 * @param factory Component factory receiving the registrations.
 */
OPENGEOLAB_RENDER_EXPORT void
registerRenderModule(Kangaroo::Util::PluginComponentFactory& factory);

} // namespace OpenGeoLab::Render
