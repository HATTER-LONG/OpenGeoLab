/**
 * @file brep_reader.hpp
 * @brief BREP file format reader implementation (internal)
 *
 * This is an internal header file, not intended for public use.
 * Provides functionality to load BREP files using Open CASCADE Technology
 * and convert them to renderable geometry data.
 */

#pragma once

#include <io/model_reader.hpp>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief BREP file format reader
 *
 * Reads Open CASCADE BREP files and converts them to triangulated
 * geometry data suitable for rendering.
 */
class BrepReader : public IModelReader {
public:
    BrepReader() = default;
    ~BrepReader() override = default;

    /**
     * @brief Product identifier for BREP reader
     */
    static std::string productId() { return "BrepReader"; }

    std::string getProductId() const override { return productId(); }

    std::vector<std::string> getSupportedExtensions() const override { return {".brep", ".brp"}; }

    bool canRead(const std::string& file_path) const override;

    std::shared_ptr<Geometry::GeometryData> read(const std::string& file_path) override;

    std::string getLastError() const override { return m_lastError; }

private:
    std::string m_lastError;
};

/**
 * @brief Factory for creating BrepReader instances
 */
class BrepReaderFactory : public IModelReaderFactory {
public:
    tObjectPtr create() const override { return std::make_unique<BrepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
