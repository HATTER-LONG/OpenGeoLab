/**
 * @file step_reader.hpp
 * @brief STEP file format reader implementation
 *
 * Handles reading of STEP (ISO 10303-21) CAD files using OpenCASCADE.
 */

#pragma once

#include "io/reader.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::IO {

/**
 * @brief Reader for STEP (Standard for Exchange of Product Data) files
 *
 * Supports both AP203 and AP214 application protocols.
 */
class StepReader : public ReaderBase {
public:
    StepReader() = default;
    ~StepReader() override = default;

    /**
     * @brief Read a STEP file and convert to geometry
     * @param filePath Path to the STEP file
     * @param progressCallback Optional progress callback
     * @return ReadResult with the loaded part
     */
    ReadResult readFile(const std::string& filePath,
                        ReadProgressCallback progressCallback = nullptr) override;

    /**
     * @brief Check if this reader can handle the given file
     */
    [[nodiscard]] bool canRead(const std::string& filePath) const override;

    /**
     * @brief Get supported file extensions
     */
    [[nodiscard]] std::vector<std::string> supportedExtensions() const override;

    /**
     * @brief Get reader description
     */
    [[nodiscard]] std::string description() const override;
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