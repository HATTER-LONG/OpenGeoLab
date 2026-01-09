/**
 * @file service.cpp
 * @brief Service registration implementation
 */

#include "service.hpp"
#include "io/model_reader.hpp"

namespace OpenGeoLab::App {

/**
 * @brief Register all built-in service factories with the global component factory
 *
 * Called during application startup to make services available for backend requests.
 */
void registerServices() {
    g_ComponentFactory.registInstanceFactoryWithID<IO::ModelReaderFactory>("ModelReader");
}

} // namespace OpenGeoLab::App