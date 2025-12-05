/**
 * @file model_reader_registry.hpp
 * @brief Model reader registry interface using dependency injection
 *
 * Provides a registry interface for model readers using the component factory
 * pattern. This replaces the Manager pattern with a more explicit dependency
 * injection approach via ComponentFactoryInjector.
 */

#pragma once

#include <io/model_reader.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Registry interface for model readers
 *
 * This interface defines the contract for accessing registered model readers.
 * The implementation manages reader registration and provides unified access
 * to different file format readers.
 */
class IModelReaderRegistry {
public:
    virtual ~IModelReaderRegistry() = default;

    /**
     * @brief Register a model reader factory with a product ID
     * @param product_id Unique identifier for the reader
     */
    virtual void registerReader(const std::string& product_id) = 0;

    /**
     * @brief Get a list of all supported file extensions
     * @return Vector of supported extensions (e.g., ".brep", ".step")
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;

    /**
     * @brief Find a suitable reader for the given file
     * @param file_path Path to the file
     * @return Unique pointer to a reader, or nullptr if no suitable reader found
     */
    virtual std::unique_ptr<IModelReader> getReaderForFile(const std::string& file_path) const = 0;

    /**
     * @brief Read a 3D model file
     * @param file_path Path to the model file
     * @param error_out Output parameter for error message
     * @return Shared pointer to geometry data, or nullptr on failure
     */
    virtual std::shared_ptr<Geometry::GeometryData> readModel(const std::string& file_path,
                                                              std::string& error_out) const = 0;

    /**
     * @brief Get the list of registered reader IDs
     * @return Vector of registered reader product IDs
     */
    virtual const std::vector<std::string>& getRegisteredReaderIds() const = 0;
};

/**
 * @brief Factory interface for creating IModelReaderRegistry instances
 *
 * This factory uses the singleton pattern (instance factory) to ensure
 * a single registry instance throughout the application lifecycle.
 */
class IModelReaderRegistryFactory
    : public Kangaroo::Util::FactoryTraits<IModelReaderRegistryFactory, IModelReaderRegistry> {
public:
    virtual ~IModelReaderRegistryFactory() = default;

    /**
     * @brief Create a new registry instance
     * @return Unique pointer to the registry
     */
    virtual tObjectPtr create() const = 0;

    /**
     * @brief Get the singleton registry instance
     * @return Shared pointer to the registry instance
     */
    virtual tObjectSharedPtr instance() const = 0;
};

/**
 * @brief Register all built-in model readers
 *
 * This function registers all built-in reader factories and the registry
 * factory with the component factory injector. Call this during application
 * initialization.
 */
void registerBuiltinModelReaders();

/**
 * @brief Get the model reader registry instance
 *
 * Convenience function to access the singleton registry instance.
 * The registry must be initialized by calling registerBuiltinModelReaders() first.
 *
 * @return Reference to the registry instance
 */
IModelReaderRegistry& getModelReaderRegistry();

} // namespace IO
} // namespace OpenGeoLab
