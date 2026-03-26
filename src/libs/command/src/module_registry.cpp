/**
 * @file module_registry.cpp
 * @brief Built-in module registration
 */

#include <opengeolab/command/module_registry.hpp>

#include <opengeolab/core/logger.hpp>
#include <opengeolab/core/module.hpp>
#include <opengeolab/io/io_module.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

namespace OpenGeoLab::Command {

void registerBuiltinModules(Kangaroo::Util::PluginComponentFactory& factory) {
    LOG_INFO("Registering built-in modules...");
    factory.bindSingleton<Core::ModuleBase, IO::IOModule>(IO::IOModule::MODULE_NAME);
    LOG_INFO("Registered module '{}'", IO::IOModule::MODULE_NAME);
}

} // namespace OpenGeoLab::Command
