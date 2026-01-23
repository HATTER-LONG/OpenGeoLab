/**
 * @file reader.hpp
 * @brief Abstract base classes for CAD file readers
 *
 * Provides the foundation for implementing file format-specific readers
 * (STEP, BREP, etc.) with progress reporting and geometry entity creation.
 */

#pragma once

#include "geometry/geometry_entity.hpp"
#include "util/occ_progress.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

#include <string>

namespace OpenGeoLab::IO {

/**
 * @brief Result structure for file read operations
 *
 * Contains success status, error information, and the loaded geometry entity.
 */
struct ReadResult {
    bool m_success{false};                    ///< Whether the read operation succeeded
    std::string m_errorMessage;               ///< Error message if failed
    Geometry::GeometryEntityPtr m_rootEntity; ///< The root entity of loaded geometry

    /**
     * @brief Create a success result with loaded entity
     * @param root_entity The root geometry entity
     * @return Success ReadResult
     */
    [[nodiscard]] static ReadResult success(Geometry::GeometryEntityPtr root_entity) {
        ReadResult result;
        result.m_success = true;
        result.m_rootEntity = std::move(root_entity);
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
 * - Creating GeometryEntity hierarchy from OCC shapes
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
    [[nodiscard]] virtual ReadResult readFile(const std::string& file_path,
                                              Util::ProgressCallback progress_callback) = 0;
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