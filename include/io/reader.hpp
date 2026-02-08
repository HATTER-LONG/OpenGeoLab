/**
 * @file reader.hpp
 * @brief Abstract base classes for CAD file readers
 *
 * Provides the foundation for implementing file format-specific readers
 * (STEP, BREP, etc.) with progress reporting and geometry entity creation.
 */

#pragma once

#include "util/progress_callback.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

#include <string>

namespace OpenGeoLab::IO {

/**
 * @brief Result structure for file read operations
 *
 * Contains success status, error information, and metadata about loaded geometry.
 */
struct ReadResult {
    bool m_success{false};      ///< Whether the read operation succeeded
    std::string m_errorMessage; ///< Error message if failed
    size_t m_entityCount{0};    ///< Number of entities created (on success)

    /**
     * @brief Create a success result
     * @param entity_count Number of entities created
     * @return Success ReadResult
     */
    [[nodiscard]] static ReadResult success(size_t entity_count = 0) {
        ReadResult result;
        result.m_success = true;
        result.m_entityCount = entity_count;
        return result;
    }

    /**
     * @brief Create a failure result with error message
     * @param message Description of what went wrong
     * @return Failure ReadResult
     */
    [[nodiscard]] static ReadResult failure(const std::string& message) {
        ReadResult result;
        result.m_success = false;
        result.m_errorMessage = message;
        return result;
    }
};

/**
 * @brief Abstract base class for CAD file readers
 *
 * Implementations should handle specific file formats (STEP, BREP, etc.)
 * and convert them into the internal geometry representation.
 *
 * Reader implementations are responsible for:
 * - File format validation
 * - Progress reporting during long operations
 * - Creating GeometryEntityImpl hierarchy from OCC shapes
 * - Error handling and meaningful error messages
 */
class ReaderBase : public Kangaroo::Util::NonCopyMoveable {
public:
    ReaderBase() = default;
    virtual ~ReaderBase() = default;

    /**
     * @brief Read a model file and return the geometry
     * @param file_path Absolute path to the file to read
     * @param progress_callback Optional callback for progress reporting
     * @return ReadResult containing the loaded geometry or error information
     *
     * @note Progress callback receives values in [0, 1] range
     * @note Callback returning false requests cancellation
     */
    [[nodiscard]] virtual ReadResult
    readFile(const std::string& file_path, // NOLINT
             Util::ProgressCallback progress_callback = Util::NO_PROGRESS_CALLBACK) = 0;
};

/**
 * @brief Factory interface for creating reader instances
 *
 * Each file format should register a factory with the component factory
 * system using a unique identifier (e.g., "BrepReader", "StepReader").
 */
class ReaderFactory : public Kangaroo::Util::FactoryTraits<ReaderFactory, ReaderBase> {
public:
    ReaderFactory() = default;
    ~ReaderFactory() = default;

    /**
     * @brief Create a new reader instance
     * @return Unique pointer to the created reader
     */
    virtual tObjectPtr create() = 0;
};

} // namespace OpenGeoLab::IO