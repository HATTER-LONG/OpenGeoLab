/**
 * @file render_data.hpp
 * @brief Discretized geometry data for OpenGL rendering
 *
 * RenderData contains triangulated mesh data extracted from OCC geometry,
 * suitable for direct consumption by OpenGL rendering pipelines.
 * Each PartEntity generates its own RenderData with a distinct color.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief RGBA color representation for rendering
 */
struct RenderColor {
    float m_r{0.7f}; ///< Red component [0, 1]
    float m_g{0.7f}; ///< Green component [0, 1]
    float m_b{0.7f}; ///< Blue component [0, 1]
    float m_a{1.0f}; ///< Alpha component [0, 1]

    /// Default constructor - light gray color
    RenderColor() = default;

    /**
     * @brief Construct color from RGBA values
     * @param r Red component [0, 1]
     * @param g Green component [0, 1]
     * @param b Blue component [0, 1]
     * @param a Alpha component [0, 1], default 1.0
     */
    RenderColor(float r, float g, float b, float a = 1.0f) : m_r(r), m_g(g), m_b(b), m_a(a) {}

    /**
     * @brief Create color from HSV values
     * @param h Hue [0, 360)
     * @param s Saturation [0, 1]
     * @param v Value [0, 1]
     * @return RenderColor in RGB
     */
    [[nodiscard]] static RenderColor fromHSV(float h, float s, float v);

    /**
     * @brief Generate a distinct color based on index
     * @param index Zero-based index for color generation
     * @param saturation Saturation level [0, 1]
     * @param value Brightness level [0, 1]
     * @return Distinct color for the given index
     *
     * Uses golden ratio to generate well-distributed hues for different parts.
     */
    [[nodiscard]] static RenderColor
    fromIndex(size_t index, float saturation = 0.7f, float value = 0.9f);
};

/**
 * @brief Vertex data for a single point in the mesh
 */
struct RenderVertex {
    float m_position[3]{0.0f, 0.0f, 0.0f};    ///< Position (x, y, z)
    float m_normal[3]{0.0f, 0.0f, 1.0f};      ///< Normal vector (nx, ny, nz)
    float m_color[4]{0.7f, 0.7f, 0.7f, 1.0f}; ///< Vertex color (r, g, b, a)

    /// Default constructor
    RenderVertex() = default;

    /**
     * @brief Construct vertex with position and normal
     * @param px Position X
     * @param py Position Y
     * @param pz Position Z
     * @param nx Normal X
     * @param ny Normal Y
     * @param nz Normal Z
     */
    RenderVertex(float px, float py, float pz, float nx, float ny, float nz)
        : m_position{px, py, pz}, m_normal{nx, ny, nz} {}

    /**
     * @brief Set vertex color
     * @param color Color to set
     */
    void setColor(const RenderColor& color) {
        m_color[0] = color.m_r;
        m_color[1] = color.m_g;
        m_color[2] = color.m_b;
        m_color[3] = color.m_a;
    }
};

/**
 * @brief Edge rendering data (for wireframe display)
 */
struct RenderEdge {
    std::vector<Geometry::Point3D> m_points;                    ///< Polyline points along the edge
    RenderColor m_color{0.1f, 0.1f, 0.1f};                      ///< Edge color (default: dark gray)
    Geometry::EntityId m_entityId{Geometry::INVALID_ENTITY_ID}; ///< Source entity ID for picking

    /// Default constructor
    RenderEdge() = default;
};

/**
 * @brief Triangulated face data for rendering
 */
struct RenderFace {
    std::vector<RenderVertex> m_vertices; ///< Vertex data
    std::vector<uint32_t> m_indices;      ///< Triangle indices (3 per triangle)
    Geometry::EntityId m_entityId{
        Geometry::INVALID_ENTITY_ID}; ///< Source face entity ID for picking

    /// Default constructor
    RenderFace() = default;

    /// Get triangle count
    [[nodiscard]] size_t triangleCount() const { return m_indices.size() / 3; }

    /// Get vertex count
    [[nodiscard]] size_t vertexCount() const { return m_vertices.size(); }
};

/**
 * @brief Complete render data for a single Part
 *
 * Contains all triangulated faces and discretized edges for rendering.
 * Use this data to feed OpenGL vertex buffers.
 */
struct PartRenderData {
    Geometry::EntityId m_partEntityId{Geometry::INVALID_ENTITY_ID}; ///< Part entity ID
    std::string m_partName;                                         ///< Part display name
    RenderColor m_baseColor;                                        ///< Base color for the part
    Geometry::BoundingBox3D m_boundingBox;                          ///< Part bounding box

    std::vector<RenderFace> m_faces; ///< Triangulated faces
    std::vector<RenderEdge> m_edges; ///< Discretized edges for wireframe

    /// Default constructor
    PartRenderData() = default;

    /// Get total triangle count across all faces
    [[nodiscard]] size_t totalTriangleCount() const;

    /// Get total vertex count across all faces
    [[nodiscard]] size_t totalVertexCount() const;

    /// Get total edge point count
    [[nodiscard]] size_t totalEdgePointCount() const;

    /**
     * @brief Merge all faces into a single vertex/index buffer
     * @param out_vertices Output vertex buffer
     * @param out_indices Output index buffer
     *
     * Useful for efficient batch rendering of the entire part.
     */
    void mergeToBuffers(std::vector<RenderVertex>& out_vertices,
                        std::vector<uint32_t>& out_indices) const;

    /**
     * @brief Get all edge points as a flat buffer for line rendering
     * @return Vector of consecutive point pairs for GL_LINES
     */
    [[nodiscard]] std::vector<float> getEdgeLineBuffer() const;
};

using PartRenderDataPtr = std::shared_ptr<PartRenderData>;

/**
 * @brief Complete render data for the entire document
 *
 * Contains render data for all parts, suitable for scene-wide rendering.
 */
struct DocumentRenderData {
    std::vector<PartRenderDataPtr> m_parts;     ///< Render data for each part
    Geometry::BoundingBox3D m_sceneBoundingBox; ///< Combined bounding box

    /// Default constructor
    DocumentRenderData() = default;

    /// Get total part count
    [[nodiscard]] size_t partCount() const { return m_parts.size(); }

    /// Get total triangle count across all parts
    [[nodiscard]] size_t totalTriangleCount() const;

    /// Recompute scene bounding box from all parts
    void updateSceneBoundingBox();
};

using DocumentRenderDataPtr = std::shared_ptr<DocumentRenderData>;

/**
 * @brief Configuration for mesh discretization
 *
 * Controls the quality of triangulation for rendering.
 */
struct TessellationParams {
    double m_linearDeflection{0.1};  ///< Linear deflection (chord height)
    double m_angularDeflection{0.5}; ///< Angular deflection in radians
    bool m_relative{true};           ///< Use relative deflection based on shape size

    /// Default constructor with reasonable defaults
    TessellationParams() = default;

    /**
     * @brief Construct with custom parameters
     * @param linear_deflection Linear deflection value
     * @param angular_deflection Angular deflection in radians
     * @param relative Use relative deflection
     */
    TessellationParams(double linear_deflection, double angular_deflection, bool relative = true)
        : m_linearDeflection(linear_deflection), m_angularDeflection(angular_deflection),
          m_relative(relative) {}

    /// High quality preset (finer mesh)
    [[nodiscard]] static TessellationParams highQuality() { return {0.01, 0.1, true}; }

    /// Medium quality preset (balanced)
    [[nodiscard]] static TessellationParams mediumQuality() { return {0.1, 0.5, true}; }

    /// Low quality preset (faster, coarser)
    [[nodiscard]] static TessellationParams lowQuality() { return {0.5, 1.0, true}; }
};

} // namespace OpenGeoLab::Render
