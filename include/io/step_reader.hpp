/**
 * @file step_reader.hpp
 * @brief STEP file format reader implementation
 *
 * Provides functionality to load STEP files using Open CASCADE Technology
 * and convert them to renderable geometry data.
 */

#pragma once

#include <io/model_reader.hpp>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief STEP file format reader
 *
 * Reads STEP/STP files and converts them to triangulated
 * geometry data suitable for rendering.
 */
class StepReader : public IModelReader {
public:
    StepReader() = default;
    ~StepReader() override = default;

    /**
     * @brief Product identifier for STEP reader
     */
    static std::string productId() { return "StepReader"; }

    std::string getProductId() const override { return productId(); }

    std::vector<std::string> getSupportedExtensions() const override { return {".step", ".stp"}; }

    bool canRead(const std::string& file_path) const override;

    std::shared_ptr<Geometry::GeometryData> read(const std::string& file_path) override;

    std::string getLastError() const override { return m_lastError; }

private:
    std::string m_lastError;
};

/**
 * @brief Factory for creating StepReader instances
 */
class StepReaderFactory : public IModelReaderFactory {
public:
    tObjectPtr create() const override { return std::make_unique<StepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
