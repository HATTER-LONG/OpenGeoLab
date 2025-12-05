/**
 * @file model_reader_registry.cpp
 * @brief Implementation of model reader registry component
 *
 * Implements the registry for model readers using the component factory
 * dependency injection pattern.
 */

#include "model_reader_registry_impl.hpp"

#include "brep_reader.hpp"
#include "step_reader.hpp"

#include <core/logger.hpp>

namespace OpenGeoLab {
namespace IO {

void ModelReaderRegistry::registerReader(const std::string& product_id) {
    m_registeredReaders.push_back(product_id);
    LOG_DEBUG("Registered model reader: {}", product_id);
}

std::vector<std::string> ModelReaderRegistry::getSupportedExtensions() const {
    std::vector<std::string> extensions;

    for(const auto& reader_id : m_registeredReaders) {
        auto reader = g_ComponentFactory.createObjectWithID<IModelReaderFactory>(reader_id);
        if(reader) {
            auto reader_exts = reader->getSupportedExtensions();
            extensions.insert(extensions.end(), reader_exts.begin(), reader_exts.end());
        }
    }

    return extensions;
}

std::unique_ptr<IModelReader>
ModelReaderRegistry::getReaderForFile(const std::string& file_path) const {
    for(const auto& reader_id : m_registeredReaders) {
        auto reader = g_ComponentFactory.createObjectWithID<IModelReaderFactory>(reader_id);
        if(reader && reader->canRead(file_path)) {
            return reader;
        }
    }
    return nullptr;
}

std::shared_ptr<Geometry::GeometryData>
ModelReaderRegistry::readModel(const std::string& file_path, std::string& error_out) const {
    auto reader = getReaderForFile(file_path);
    if(!reader) {
        error_out = "No suitable reader found for file: " + file_path;
        LOG_ERROR(error_out);
        return nullptr;
    }

    auto geometry = reader->read(file_path);
    if(!geometry) {
        error_out = reader->getLastError();
    }

    return geometry;
}

void registerBuiltinModelReaders() {
    LOG_INFO("Registering built-in model readers");

    // Register the registry factory as an instance factory (singleton)
    g_ComponentFactory.registInstanceFactory<ModelReaderRegistryFactory>();

    // Register individual reader factories
    g_ComponentFactory.registFactoryWithID<BrepReaderFactory>(BrepReader::productId());
    g_ComponentFactory.registFactoryWithID<StepReaderFactory>(StepReader::productId());

    // Register readers with the registry
    auto& registry = getModelReaderRegistry();
    registry.registerReader(BrepReader::productId());
    registry.registerReader(StepReader::productId());

    LOG_INFO("Registered {} model readers", registry.getRegisteredReaderIds().size());
}

IModelReaderRegistry& getModelReaderRegistry() {
    return *g_ComponentFactory.getInstanceObject<ModelReaderRegistryFactory>();
}

} // namespace IO
} // namespace OpenGeoLab
