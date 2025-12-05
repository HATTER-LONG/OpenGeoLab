/**
 * @file brep_reader.hpp
 * @brief BREP file format reader implementation (internal)
 *
 * This is an internal header file, not intended for public use.
 * Provides functionality to load BREP files using Open CASCADE Technology
 * and convert them to renderable geometry data.
 *
 * BREP (Boundary Representation) is a method for representing 3D shapes
 * using the limits of their surfaces. Open CASCADE's native BREP format
 * stores complete topological and geometric information.
 */

#pragma once

#include <io/model_reader.hpp>

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab {
namespace IO {

/**
 * @brief Vertex data structure for mesh generation
 *
 * Contains position (x, y, z) and normal (nx, ny, nz) data for a single vertex.
 * Used for vertex deduplication during mesh extraction.
 */
struct VertexData {
    float m_posX = 0.0f;    ///< X position coordinate
    float m_posY = 0.0f;    ///< Y position coordinate
    float m_posZ = 0.0f;    ///< Z position coordinate
    float m_normalX = 0.0f; ///< X component of normal vector
    float m_normalY = 0.0f; ///< Y component of normal vector
    float m_normalZ = 0.0f; ///< Z component of normal vector

    /**
     * @brief Compare two vertices for equality
     * @param other The other vertex to compare
     * @return true if vertices are approximately equal (within epsilon)
     */
    bool operator==(const VertexData& other) const;
};

/**
 * @brief Hash function for VertexData
 *
 * Provides hash computation for use in std::unordered_map
 * for vertex deduplication.
 */
struct VertexDataHash {
    /**
     * @brief Compute hash value for a vertex
     * @param vertex The vertex to hash
     * @return Hash value based on position coordinates
     */
    std::size_t operator()(const VertexData& vertex) const;
};

/**
 * @brief BREP file format reader
 *
 * Reads Open CASCADE BREP files and converts them to triangulated
 * geometry data suitable for rendering. Uses BRepTools for file I/O
 * and BRepMesh_IncrementalMesh for triangulation.
 *
 * Supported file extensions: .brep, .brp
 */
class BrepReader : public IModelReader {
public:
    BrepReader() = default;
    ~BrepReader() override = default;

    /**
     * @brief Get the product identifier for BREP reader
     * @return Product ID string "BrepReader"
     */
    static std::string productId() { return "BrepReader"; }

    /**
     * @brief Get the product identifier for this reader instance
     * @return Product ID string "BrepReader"
     */
    std::string getProductId() const override { return productId(); }

    /**
     * @brief Get the list of supported file extensions
     * @return Vector containing ".brep" and ".brp"
     */
    std::vector<std::string> getSupportedExtensions() const override { return {".brep", ".brp"}; }

    /**
     * @brief Check if this reader can handle the given file
     * @param file_path Path to the file to check
     * @return true if the file has a supported extension
     */
    bool canRead(const std::string& file_path) const override;

    /**
     * @brief Read a BREP file and convert to geometry data
     *
     * Loads the BREP file, performs mesh triangulation, and extracts
     * vertex and index data for rendering.
     *
     * @param file_path Path to the BREP file
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
     * @brief Load a BREP file and return the shape
     * @param file_path Path to the BREP file
     * @param shape Output shape containing the loaded geometry
     * @return true on success, false on failure
     */
    bool loadBrepFile(const std::string& file_path, TopoDS_Shape& shape);

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
 * @brief Factory for creating BrepReader instances
 *
 * Used by the component factory system to create BrepReader objects
 * on demand.
 */
class BrepReaderFactory : public IModelReaderFactory {
public:
    /**
     * @brief Create a new BrepReader instance
     * @return Unique pointer to the new BrepReader
     */
    tObjectPtr create() const override { return std::make_unique<BrepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
