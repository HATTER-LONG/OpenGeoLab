/**
 * @file brep_reader.hpp
 * @brief BREP file format reader implementation
 *
 * Handles reading of OpenCASCADE BREP (Boundary Representation) files.
 */

#pragma once

#include "io/reader.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::IO {

/**
 * @brief Reader for OpenCASCADE BREP files
 *
 * BREP is the native format for OpenCASCADE boundary representation data.
 */
class BrepReader : public ReaderBase {
public:
    BrepReader() = default;
    ~BrepReader() override = default;

    /**
     * @brief Read a BREP file and convert to geometry
     * @param filePath Path to the BREP file
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
 * @brief Factory for creating BrepReader instances
 */
class BrepReaderFactory : public ReaderFactory {
public:
    BrepReaderFactory() = default;
    ~BrepReaderFactory() = default;

    tObjectPtr create() override { return std::make_unique<BrepReader>(); }
};

} // namespace OpenGeoLab::IO