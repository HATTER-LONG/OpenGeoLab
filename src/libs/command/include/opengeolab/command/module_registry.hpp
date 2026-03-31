/**
 * @file module_registry.hpp
 * @brief Registers all built-in modules into a PluginComponentFactory
 */

#pragma once

#include <opengeolab/command/command_export.hpp>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Command {

/**
 * @brief Register all built-in modules (io, ...) in the given factory.
 *
 * Call once at application startup before dispatching commands.
 *
 * @param factory The component factory to register modules into
 */
OPENGEOLAB_COMMAND_EXPORT void
registerBuiltinModules(Kangaroo::Util::PluginComponentFactory& factory);

} // namespace OpenGeoLab::Command
