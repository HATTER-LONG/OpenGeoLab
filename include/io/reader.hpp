/**
 * @file reader.hpp
 * @brief Base interface for CAD file readers
 *
 * Defines the abstract reader interface and factory for loading
 * 3D model files into the geometry system.
 */

#pragma once

#include "geometry/geometry_entity.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

#include <functional>
#include <memory>
#include <string>

namespace OpenGeoLab::IO {

/**
 * @brief Read result containing the loaded geometry and status information
 */
struct ReadResult {
    bool success{false};                        ///< Whether the read operation succeeded
    std::string errorMessage;                   ///< Error message if failed
    std::shared_ptr<Geometry::PartEntity> part; ///< The loaded part (if successful)

    /**
     * @brief Create a success result
     */
    static ReadResult Success(std::shared_ptr<Geometry::PartEntity> loadedPart) {
        ReadResult result;
        result.success = true;
        result.part = std::move(loadedPart);
        return result;
    }

    /**
     * @brief Create a failure result
     */
    static ReadResult Failure(const std::string& message) {
        ReadResult result;
        result.success = false;
        result.errorMessage = message;
        return result;
    }
};

/**
 * @brief Progress callback for read operations
 * @param progress Progress value in [0.0, 1.0]
 * @param message Status message
 * @return false to cancel the operation
 */
using ReadProgressCallback = std::function<bool(double progress, const std::string& message)>;

/**
 * @brief Abstract base class for CAD file readers
 *
 * Implementations should handle specific file formats (STEP, BREP, etc.)
 * and convert them into the internal geometry representation.
 */
class ReaderBase : public Kangaroo::Util::NonCopyMoveable {
public:
    ReaderBase() = default;
    virtual ~ReaderBase() = default;

    /**
     * @brief Read a model file and return the geometry
     * @param filePath Path to the file to read
     * @param progressCallback Optional callback for progress reporting
     * @return ReadResult containing the loaded part or error information
     */
    virtual ReadResult readFile(const std::string& filePath,
                                ReadProgressCallback progressCallback = nullptr) = 0;

    /**
     * @brief Check if this reader can handle the given file
     * @param filePath Path to the file to check
     * @return true if this reader supports the file format
     */
    virtual bool canRead(const std::string& filePath) const = 0;

    /**
     * @brief Get the list of supported file extensions
     * @return Vector of extensions (e.g., {".step", ".stp"})
     */
    virtual std::vector<std::string> supportedExtensions() const = 0;

    /**
     * @brief Get a human-readable description of this reader
     */
    virtual std::string description() const = 0;
};

/**
 * @brief Factory interface for creating reader instances
 */
class ReaderFactory : public Kangaroo::Util::FactoryTraits<ReaderFactory, ReaderBase> {
public:
    ReaderFactory() = default;
    virtual ~ReaderFactory() = default;

    virtual tObjectPtr create() = 0;
};

} // namespace OpenGeoLab::IO