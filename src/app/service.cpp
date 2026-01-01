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

void registerServices() {
    g_ComponentFactory.registInstanceFactoryWithID<IO::ModelReaderFactory>("ModelReader");
    g_ComponentFactory.registInstanceFactoryWithID<IO::BrepReaderFactory>("BrepReader");
    g_ComponentFactory.registInstanceFactoryWithID<IO::StepReaderFactory>("StepReader");
}

} // namespace OpenGeoLab::App
