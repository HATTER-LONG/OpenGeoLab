/**
 * @file service.cpp
 * @brief Service registration implementation
 */

#include "service.hpp"
#include "geometry/geometry_service.hpp"
#include "io/reader_service.hpp"
#include "render/render_service.hpp"
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {

/**
 * @brief Register all built-in service factories with the global component factory
 *
 * Called during application startup to make services available for backend requests.
 */
void registerServices() {
    IO::registerServices();
    Geometry::registerServices();
    Render::registerServices();
}

} // namespace OpenGeoLab::App