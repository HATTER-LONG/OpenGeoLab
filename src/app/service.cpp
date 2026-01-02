/**
 * @file service.cpp
 * @brief Service registration implementation.
 *
 * Registers all available service components with the component factory.
 */
#include "service.hpp"
#include "geometry/geometry_builder.hpp"
#include "geometry/geometry_editor.hpp"
#include "io/brep_reader.hpp"
#include "io/model_reader.hpp"
#include "io/step_reader.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {

void registerServices() {
    // IO services
    g_ComponentFactory.registInstanceFactoryWithID<IO::ModelReaderFactory>("ModelReader");
    g_ComponentFactory.registFactoryWithID<IO::BrepReaderFactory>("BrepReader");
    g_ComponentFactory.registFactoryWithID<IO::StepReaderFactory>("StepReader");

    // Geometry creation services
    g_ComponentFactory.registInstanceFactoryWithID<Geometry::GeometryBuilderFactory>("AddBox");

    // Geometry editing services
    g_ComponentFactory.registInstanceFactoryWithID<Geometry::GeometryEditorFactory>("Trim");
    g_ComponentFactory.registInstanceFactoryWithID<Geometry::GeometryEditorFactory>("Offset");
}

} // namespace OpenGeoLab::App
