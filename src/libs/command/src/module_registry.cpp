/**
 * @file module_registry.cpp
 * @brief Implements the central module registration by delegating to each module's register
 * function.
 */

#include <opengeolab/command/module_registry.hpp>

#include <opengeolab/geometry/geometry_registration.hpp>

namespace OpenGeoLab::Command {

void registerAllModules(Kangaroo::Util::PluginComponentFactory& factory) {
    Geometry::registerGeometryModule(factory);
}

} // namespace OpenGeoLab::Command
