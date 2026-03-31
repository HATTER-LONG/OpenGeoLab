/**
 * @file io_module.cpp
 * @brief IOModule — registers I/O actions into the factory
 */

#include <opengeolab/io/io_module.hpp>
#include <opengeolab/io/read_brep_action.hpp>

namespace OpenGeoLab::IO {

IOModule::IOModule(Kangaroo::Util::PluginComponentFactory& factory)
    : ModuleBase(MODULE_NAME, "I/O module for reading and writing geometry files.", factory) {
    registerAction<ReadBrepAction>();
}

IOModule::~IOModule() = default;

} // namespace OpenGeoLab::IO
