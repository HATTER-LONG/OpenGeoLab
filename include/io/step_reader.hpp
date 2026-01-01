/**
 * @file step_reader.hpp
 * @brief STEP format geometry reader component
 */
#pragma once

#include "geometry_data.hpp"

#include <kangaroo/util/component_factory.hpp>

#include <memory>
#include <string>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Reader component for STEP (ISO 10303) format files
 *
 * Handles parsing and conversion of STEP CAD files to internal geometry representation.
 * Supports common STEP variants (AP203, AP214, AP242).
 */
class StepReader {
public:
    StepReader() = default;
    virtual ~StepReader() = default;

    /**
     * @brief Read STEP file and extract geometry data
     * @param file_path Path to .step or .stp file
     * @return Geometry data on success, nullptr on failure
     * @warning Not thread-safe for concurrent reads from same instance
     */
    virtual GeometryDataPtr read(const std::string& file_path);

private:
    /**
     * @brief Parse STEP file format
     * @param file_path Input file path
     * @param data Output geometry data
     * @return true on successful parse
     */
    bool parseStepFile(const std::string& file_path, GeometryData& data);
};

/**
 * @brief Factory for StepReader singleton
 */
class StepReaderFactory : public Kangaroo::Util::FactoryTraits<StepReaderFactory, StepReader> {
public:
    StepReaderFactory() = default;
    virtual ~StepReaderFactory() = default;

    virtual tObjectSharedPtr instance() const { return std::make_shared<StepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
