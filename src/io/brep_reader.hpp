/**
 * @file brep_reader.hpp
 * @brief BREP file format reader implementation
 */

#pragma once

#include "io/reader.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::IO {

/**
 * @brief Reader for OpenCASCADE BREP file format
 *
 * Supports .brep and .brp file extensions.
 * Creates a geometry entity hierarchy from the loaded shape.
 */
class BrepReader : public ReaderBase {
public:
    BrepReader() = default;
    ~BrepReader() override = default;

    /**
     * @brief Read a BREP file and create geometry entities
     * @param file_path Path to the BREP file
     * @param progress_callback Progress reporting callback
     * @return ReadResult with root entity or error information
     */
    ReadResult readFile(const std::string& file_path,
                        Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating BrepReader instances
 */
class BrepReaderFactory : public ReaderFactory {
public:
    BrepReaderFactory() = default;
    ~BrepReaderFactory() = default;

    tObjectPtr create() override { return std::make_unique<BrepReader>(); }
};

} // namespace OpenGeoLab::IO