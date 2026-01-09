#include "service.hpp"
#include "io/model_reader.hpp"

namespace OpenGeoLab::App {
void registerServices() {
    g_ComponentFactory.registInstanceFactoryWithID<IO::ModelReaderFactory>("ModelReader");
}
} // namespace OpenGeoLab::App