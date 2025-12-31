#include "service.hpp"
#include "io/model_reader.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::App {
void registeServices() {
    g_ComponentFactory.registInstanceFactoryWithID<IO::ModelReaderFactory>("ModelReader");
}
} // namespace OpenGeoLab::App
