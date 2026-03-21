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

// TODO: implementation in later tasks
OPENGEOLAB_RENDER_EXPORT void
registerRenderModule(Kangaroo::Util::PluginComponentFactory& factory);

} // namespace OpenGeoLab::Render
