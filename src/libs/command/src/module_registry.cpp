/**
 * @file module_registry.cpp
 * @brief Built-in module registration
 */

#include <opengeolab/command/module_registry.hpp>

#include <opengeolab/core/logger.hpp>
#include <opengeolab/core/module.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/io/io_module.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>

#include <algorithm>
#include <functional>

namespace OpenGeoLab::Command {

void registerBuiltinModules(Kangaroo::Util::PluginComponentFactory& factory) {
    auto existing = factory.listFactories<Core::ModuleBase>();
    auto is_registered = [&](std::string_view name) {
        return std::ranges::any_of(existing,
                                   [&](const auto& info) { return info.m_moduleName == name; });
    };

    if(!is_registered(IO::IOModule::MODULE_NAME)) {
        factory.bindSingleton<Core::ModuleBase, IO::IOModule>(IO::IOModule::MODULE_NAME,
                                                              std::ref(factory));
        LOG_INFO("Registered module '{}'", IO::IOModule::MODULE_NAME);
    }
    if(!is_registered(Geometry::GeometryModule::MODULE_NAME)) {
        factory.bindSingleton<Core::ModuleBase, Geometry::GeometryModule>(
            Geometry::GeometryModule::MODULE_NAME, std::ref(factory));
        LOG_INFO("Registered module '{}'", Geometry::GeometryModule::MODULE_NAME);
    }
}

} // namespace OpenGeoLab::Command
