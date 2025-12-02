/**
 * @file model_reader_manager.cpp
 * @brief Implementation of model reader manager
 *
 * Manages registration and access to all model reader components.
 */

#include <io/model_reader_manager.hpp>

#include <io/brep_reader.hpp>
#include <io/step_reader.hpp>

#include <core/logger.hpp>

namespace OpenGeoLab {
namespace IO {

ModelReaderManager& ModelReaderManager::instance() {
    static ModelReaderManager instance;
    return instance;
}

void ModelReaderManager::registerBuiltinReaders() {
    LOG_INFO("Registering built-in model readers");

    // Register BREP reader
    registerReader<BrepReaderFactory>(BrepReader::productId());

    // Register STEP reader
    registerReader<StepReaderFactory>(StepReader::productId());

    LOG_INFO("Registered {} model readers", m_registeredReaders.size());
}

std::vector<std::string> ModelReaderManager::getSupportedExtensions() const {
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
ModelReaderManager::getReaderForFile(const std::string& file_path) const {
    for(const auto& reader_id : m_registeredReaders) {
        auto reader = g_ComponentFactory.createObjectWithID<IModelReaderFactory>(reader_id);
        if(reader && reader->canRead(file_path)) {
            return reader;
        }
    }
    return nullptr;
}

std::shared_ptr<Geometry::GeometryData>
ModelReaderManager::readModel(const std::string& file_path, std::string& error_out) const {
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

} // namespace IO
} // namespace OpenGeoLab
