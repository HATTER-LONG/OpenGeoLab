/**
 * @file brep_reader.hpp
 * @brief BREP format geometry reader component
 */
#pragma once

#include "geometry_data.hpp"

#include <kangaroo/util/component_factory.hpp>

#include <memory>
#include <string>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Reader component for BREP (Boundary Representation) format files
 *
 * Handles parsing and conversion of BREP CAD files to internal geometry representation.
 */
class BrepReader {
public:
    BrepReader() = default;
    virtual ~BrepReader() = default;

    /**
     * @brief Read BREP file and extract geometry data
     * @param file_path Path to .brep file
     * @return Geometry data on success, nullptr on failure
     * @warning Not thread-safe for concurrent reads from same instance
     */
    virtual GeometryDataPtr read(const std::string& file_path);

private:
    /**
     * @brief Parse BREP file format
     * @param file_path Input file path
     * @param data Output geometry data
     * @return true on successful parse
     */
    bool parseBrepFile(const std::string& file_path, GeometryData& data);
};

/**
 * @brief Factory for BrepReader singleton
 */
class BrepReaderFactory : public Kangaroo::Util::FactoryTraits<BrepReaderFactory, BrepReader> {
public:
    BrepReaderFactory() = default;
    virtual ~BrepReaderFactory() = default;

    virtual tObjectSharedPtr instance() const { return std::make_shared<BrepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
