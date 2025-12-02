/**
 * @file model_reader_manager.hpp
 * @brief Manager for model reader components
 *
 * Provides a unified interface for registering and accessing model readers.
 * Uses the component factory pattern to manage different file format readers.
 */

#pragma once

#include <io/model_reader.hpp>

#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Manager for model reader components
 *
 * This singleton class manages all registered model readers and provides
 * a unified interface for reading 3D model files of various formats.
 */
class ModelReaderManager {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the manager instance
     */
    static ModelReaderManager& instance();

    /**
     * @brief Register all built-in model readers
     *
     * Call this method during application initialization to register
     * all supported file format readers.
     */
    void registerBuiltinReaders();

    /**
     * @brief Register a custom model reader factory
     * @tparam FactoryType The factory type to register
     * @param product_id Unique identifier for the reader
     */
    template <typename FactoryType> void registerReader(const std::string& product_id) {
        g_ComponentFactory.registFactoryWithID<FactoryType>(product_id);
        m_registeredReaders.push_back(product_id);
    }

    /**
     * @brief Get a list of all supported file extensions
     * @return Vector of supported extensions (e.g., ".brep", ".step")
     */
    std::vector<std::string> getSupportedExtensions() const;

    /**
     * @brief Find a suitable reader for the given file
     * @param file_path Path to the file
     * @return Unique pointer to a reader, or nullptr if no suitable reader found
     */
    std::unique_ptr<IModelReader> getReaderForFile(const std::string& file_path) const;

    /**
     * @brief Read a 3D model file
     * @param file_path Path to the model file
     * @param error_out Output parameter for error message
     * @return Shared pointer to geometry data, or nullptr on failure
     */
    std::shared_ptr<Geometry::GeometryData> readModel(const std::string& file_path,
                                                      std::string& error_out) const;

private:
    ModelReaderManager() = default;
    ~ModelReaderManager() = default;

    ModelReaderManager(const ModelReaderManager&) = delete;
    ModelReaderManager& operator=(const ModelReaderManager&) = delete;

    std::vector<std::string> m_registeredReaders;
};

} // namespace IO
} // namespace OpenGeoLab
