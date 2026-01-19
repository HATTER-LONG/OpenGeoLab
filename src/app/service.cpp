/**
 * @file service.cpp
 * @brief Service registration implementation
 */

#include "service.hpp"
#include "io/brep_reader.hpp"
#include "io/model_reader.hpp"
#include "io/step_reader.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {

/**
 * @brief Register all built-in service factories with the global component factory
 *
 * Called during application startup to make services available for backend requests.
 */
void registerServices() {
    // Register the model reader service (singleton)
    g_ComponentFactory.registInstanceFactoryWithID<IO::ModelReaderFactory>("ModelReader");

    // Register file format readers (factory pattern)
    g_ComponentFactory.registFactoryWithID<IO::BrepReaderFactory>("BrepReader");
    g_ComponentFactory.registFactoryWithID<IO::StepReaderFactory>("StepReader");
}

} // namespace OpenGeoLab::App