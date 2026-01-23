/**
 * @file step_reader.hpp
 * @brief STEP file format reader implementation
 */

#pragma once

#include "io/reader.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::IO {

/**
 * @brief Reader for STEP (ISO 10303-21) file format
 *
 * Supports .step and .stp file extensions.
 * Creates a geometry entity hierarchy from the loaded shape.
 */
class StepReader : public ReaderBase {
public:
    StepReader() = default;
    ~StepReader() override = default;

    /**
     * @brief Read a STEP file and create geometry entities
     * @param file_path Path to the STEP file
     * @param progress_callback Progress reporting callback
     * @return ReadResult with root entity or error information
     */
    ReadResult readFile(const std::string& file_path,
                        Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating StepReader instances
 */
class StepReaderFactory : public ReaderFactory {
public:
    StepReaderFactory() = default;
    ~StepReaderFactory() = default;

    tObjectPtr create() override { return std::make_unique<StepReader>(); }
};

} // namespace OpenGeoLab::IO