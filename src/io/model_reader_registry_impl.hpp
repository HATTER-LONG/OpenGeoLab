/**
 * @file model_reader_registry_impl.hpp
 * @brief Model reader registry implementation (internal)
 *
 * This is an internal header file, not intended for public use.
 * Contains the concrete implementation of the model reader registry.
 */

#pragma once

#include <io/model_reader_registry.hpp>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Concrete implementation of IModelReaderRegistry
 *
 * This class manages registration and access to model readers.
 * It uses the component factory to create reader instances on demand.
 */
class ModelReaderRegistry : public IModelReaderRegistry {
public:
    ModelReaderRegistry() = default;
    ~ModelReaderRegistry() override = default;

    /**
     * @brief Register a model reader factory with a product ID
     *
     * The factory must already be registered with ComponentFactoryInjector
     * before calling this method. This method only tracks the reader ID
     * for iteration purposes.
     *
     * @param product_id Unique identifier for the reader
     */
    void registerReader(const std::string& product_id) override;

    std::vector<std::string> getSupportedExtensions() const override;

    std::unique_ptr<IModelReader> getReaderForFile(const std::string& file_path) const override;

    std::shared_ptr<Geometry::GeometryData> readModel(const std::string& file_path,
                                                      std::string& error_out) const override;

    const std::vector<std::string>& getRegisteredReaderIds() const override {
        return m_registeredReaders;
    }

private:
    std::vector<std::string> m_registeredReaders;
};

/**
 * @brief Concrete factory for ModelReaderRegistry
 *
 * Implements the instance factory pattern for singleton access
 * while allowing creation of new instances for testing.
 */
class ModelReaderRegistryFactory : public IModelReaderRegistryFactory {
public:
    tObjectPtr create() const override { return std::make_unique<ModelReaderRegistry>(); }

    tObjectSharedPtr instance() const override {
        static auto s_instance = std::make_shared<ModelReaderRegistry>();
        return s_instance;
    }
};

} // namespace IO
} // namespace OpenGeoLab
