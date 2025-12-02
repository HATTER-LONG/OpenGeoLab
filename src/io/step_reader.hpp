/**
 * @file step_reader.hpp
 * @brief STEP file format reader implementation (internal)
 *
 * This is an internal header file, not intended for public use.
 * Provides functionality to load STEP files using Open CASCADE Technology
 * and convert them to renderable geometry data.
 *
 * STEP (Standard for the Exchange of Product Data) is an ISO standard
 * (ISO 10303) for the computer-interpretable representation and exchange
 * of product manufacturing information.
 */

#pragma once

#include <io/model_reader.hpp>

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief STEP file format reader
 *
 * Reads STEP/STP files and converts them to triangulated
 * geometry data suitable for rendering. Uses Open CASCADE's
 * STEPControl_Reader for parsing STEP files.
 *
 * Supported file extensions: .step, .stp
 */
class StepReader : public IModelReader {
public:
    StepReader() = default;
    ~StepReader() override = default;

    /**
     * @brief Get the product identifier for STEP reader
     * @return Product ID string "StepReader"
     */
    static std::string productId() { return "StepReader"; }

    /**
     * @brief Get the product identifier for this reader instance
     * @return Product ID string "StepReader"
     */
    std::string getProductId() const override { return productId(); }

    /**
     * @brief Get the list of supported file extensions
     * @return Vector containing ".step" and ".stp"
     */
    std::vector<std::string> getSupportedExtensions() const override { return {".step", ".stp"}; }

    /**
     * @brief Check if this reader can handle the given file
     * @param file_path Path to the file to check
     * @return true if the file has a supported extension
     */
    bool canRead(const std::string& file_path) const override;

    /**
     * @brief Read a STEP file and convert to geometry data
     *
     * Loads the STEP file, performs mesh triangulation, and extracts
     * vertex and index data for rendering.
     *
     * @param file_path Path to the STEP file
     * @return Shared pointer to geometry data, or nullptr on failure
     */
    std::shared_ptr<Geometry::GeometryData> read(const std::string& file_path) override;

    /**
     * @brief Get the last error message
     * @return Error message string, empty if no error occurred
     */
    std::string getLastError() const override { return m_lastError; }

private:
    /**
     * @brief Load a STEP file and return the shape
     * @param file_path Path to the STEP file
     * @param shape Output shape containing the loaded geometry
     * @return true on success, false on failure
     */
    bool loadStepFile(const std::string& file_path, TopoDS_Shape& shape);

    /**
     * @brief Perform mesh triangulation on the shape
     *
     * Uses BRepMesh_IncrementalMesh to generate triangulated mesh
     * with configurable linear and angular deflection parameters.
     *
     * @param shape The shape to triangulate (modified in place)
     * @return true on success, false on failure
     */
    bool triangulateShape(TopoDS_Shape& shape);

    /**
     * @brief Extract triangle data from a triangulated shape
     *
     * Iterates over all faces in the shape, extracts triangles,
     * computes normals, and builds vertex/index buffers with
     * deduplication.
     *
     * @param shape The triangulated shape
     * @param vertex_data Output buffer for vertex data (pos, normal, color)
     * @param index_data Output buffer for triangle indices
     * @return true on success, false on failure
     */
    bool extractTriangleData(const TopoDS_Shape& shape,
                             std::vector<float>& vertex_data,
                             std::vector<unsigned int>& index_data);

    std::string m_lastError; ///< Last error message
};

/**
 * @brief Factory for creating StepReader instances
 *
 * Used by the component factory system to create StepReader objects
 * on demand.
 */
class StepReaderFactory : public IModelReaderFactory {
public:
    /**
     * @brief Create a new StepReader instance
     * @return Unique pointer to the new StepReader
     */
    tObjectPtr create() const override { return std::make_unique<StepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
