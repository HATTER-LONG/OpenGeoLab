#pragma once
#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <string>

#include "util/occ_progress.hpp"

namespace OpenGeoLab::IO {
/**
 * @brief Read result containing the loaded geometry and status information
 */
struct ReadResult {
    bool m_success{false};        ///< Whether the read operation succeeded
    std::string m_errorMessage;   ///< Error message if failed
    std::shared_ptr<void> m_part; ///< The loaded part (if successful)

    /**
     * @brief Create a success result
     */
    [[nodiscard]] static ReadResult success(std::shared_ptr<void> loaded_part) {
        ReadResult result;
        result.m_success = true;
        result.m_part = std::move(loaded_part);
        return result;
    }

    /**
     * @brief Create a failure result
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
    [[nodiscard]] virtual ReadResult readFile(const std::string& file_path,
                                              Util::ProgressCallback progress_callback) = 0;
};
/**
 * @brief Factory interface for creating reader instances
 */
class ReaderFactory : public Kangaroo::Util::FactoryTraits<ReaderFactory, ReaderBase> {
public:
    ReaderFactory() = default;
    ~ReaderFactory() = default;

    virtual tObjectPtr create() = 0;
};
} // namespace OpenGeoLab::IO