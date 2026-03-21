/**
 * @file module_registry.hpp
 * @brief Declares the central entry point for registering all known module services.
 */

#pragma once

#include <opengeolab/command/command_export.hpp>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::Command {

/**
 * @brief Registers all known module services and their actions with the given factory.
 *
 * This is the single place that enumerates every concrete module the application supports.
 * When a new module is introduced, add its registration call here.
 *
 * @param factory Component factory receiving the registrations.
 */
OPENGEOLAB_COMMAND_EXPORT void registerAllModules(Kangaroo::Util::PluginComponentFactory& factory);

} // namespace OpenGeoLab::Command
