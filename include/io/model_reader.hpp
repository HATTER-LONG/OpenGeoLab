/**
 * @file model_reader.hpp
 * @brief Abstract interface and factory for 3D model file readers
 *
 * Defines the interface for reading different 3D model file formats (BREP, STEP, etc.)
 * using the component factory pattern for extensibility.
 */

#pragma once

#include <geometry/geometry.hpp>
#include <kangaroo/util/component_factory.hpp>

#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Abstract interface for 3D model file readers
 *
 * This interface defines the contract that all model readers must implement.
 * Each reader handles a specific file format (BREP, STEP, etc.).
 */
class IModelReader {
public:
    IModelReader() = default;
    virtual ~IModelReader() = default;

    /**
     * @brief Get the unique product identifier for this reader type
     * @return std::string Unique identifier string
     */
    virtual std::string getProductId() const = 0;

    /**
     * @brief Get the file extensions supported by this reader
     * @return std::vector<std::string> List of supported extensions (e.g., ".brep", ".step")
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;

    /**
     * @brief Check if this reader can handle the given file
     * @param file_path Path to the file
     * @return true if this reader supports the file format
     */
    virtual bool canRead(const std::string& file_path) const = 0;

    /**
     * @brief Read a 3D model file and convert to geometry data
     * @param file_path Path to the model file
     * @return Shared pointer to geometry data, or nullptr on failure
     */
    virtual std::shared_ptr<Geometry::GeometryData> read(const std::string& file_path) = 0;

    /**
     * @brief Get the last error message
     * @return Error message string, empty if no error
     */
    virtual std::string getLastError() const = 0;
};

/**
 * @brief Factory interface for creating IModelReader objects
 *
 * This factory defines the creation method for model readers.
 * Each file format has its own factory implementation.
 */
class IModelReaderFactory
    : public Kangaroo::Util::FactoryTraits<IModelReaderFactory, IModelReader> {
public:
    virtual ~IModelReaderFactory() = default;

    /**
     * @brief Create a model reader instance
     * @return Unique pointer to the model reader
     */
    virtual tObjectPtr create() const = 0;
};

} // namespace IO
} // namespace OpenGeoLab
