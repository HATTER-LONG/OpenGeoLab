/**
 * @file geometry_module.cpp
 * @brief GeometryModule — registers geometry actions into the factory
 */

#include <opengeolab/geometry/create_box_action.hpp>
#include <opengeolab/geometry/geometry_module.hpp>

namespace OpenGeoLab::Geometry {

GeometryModule::GeometryModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "Geometry creation and manipulation module.", factory) {
    registerAction<CreateBoxAction>();
}

GeometryModule::~GeometryModule() = default;

} // namespace OpenGeoLab::Geometry
