/**
 * @file render_data.hpp
 * @brief Render data structures for OpenGL geometry visualization
 *
 * Defines data structures for transferring geometry to the rendering layer.
 * These structures are designed for efficient GPU upload and OpenGL rendering.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "mesh/mesh_types.hpp"
#include "render/pick_entity_type.hpp"

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Simple RGBA color used by the render layer
 */
struct RenderColor {
    float m_r{0.8f}; ///< Red component [0, 1]
    float m_g{0.8f}; ///< Green component [0, 1]
    float m_b{0.8f}; ///< Blue component [0, 1]
    float m_a{1.0f}; ///< Alpha component [0, 1]
};

/**
 * @brief Vertex data for rendering with position, normal, and color
 *
 * Packed structure for efficient GPU memory usage.
 * Layout: position (3 floats), normal (3 floats), color (4 floats)
 */
struct RenderVertex {
    float m_position[3]{0.0f, 0.0f, 0.0f};       ///< Vertex position (x, y, z)
    float m_normal[3]{0.0f, 0.0f, 1.0f};         ///< Vertex normal for lighting
    RenderColor m_color{0.8f, 0.8f, 0.8f, 1.0f}; ///< RGBA color

    RenderVertex() = default;

    /**
     * @brief Construct from position
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    RenderVertex(float x, float y, float z) {
        m_position[0] = x;
        m_position[1] = y;
        m_position[2] = z;
    }

    /**
     * @brief Construct from position and normal
     * @param px Position X
     * @param py Position Y
     * @param pz Position Z
     * @param nx Normal X
     * @param ny Normal Y
     * @param nz Normal Z
     */
    RenderVertex(float px, float py, float pz, float nx, float ny, float nz) { // NOLINT
        m_position[0] = px;
        m_position[1] = py;
        m_position[2] = pz;
        m_normal[0] = nx;
        m_normal[1] = ny;
        m_normal[2] = nz;
    }

    /**
     * @brief Set vertex color
     * @param r Red component [0, 1]
     * @param g Green component [0, 1]
     * @param b Blue component [0, 1]
     * @param a Alpha component [0, 1]
     */
    void setColor(float r, float g, float b, float a = 1.0f) {
        m_color.m_r = r;
        m_color.m_g = g;
        m_color.m_b = b;
        m_color.m_a = a;
    }
};

/**
 * @brief Render primitive type enumeration
 */
enum class RenderPrimitiveType : uint8_t {
    Points = 0,        ///< GL_POINTS
    Lines = 1,         ///< GL_LINES
    LineStrip = 2,     ///< GL_LINE_STRIP
    Triangles = 3,     ///< GL_TRIANGLES
    TriangleStrip = 4, ///< GL_TRIANGLE_STRIP
    TriangleFan = 5    ///< GL_TRIANGLE_FAN
};

struct BaseRenderData {
    PickEntityType m_pickType{PickEntityType::None};                     ///< Type for GPU picking
    RenderPrimitiveType m_primitiveType{RenderPrimitiveType::Triangles}; ///< Primitive type

    std::vector<RenderVertex> m_vertices; ///< Vertex data
    std::vector<uint32_t> m_indices;      ///< Index data (empty for non-indexed draw)

    Geometry::BoundingBox3D m_boundingBox; ///< Mesh bounding box

    RenderColor m_baseColor{};     ///< Base color associated with this mesh (informational)
    RenderColor m_hoverColor{};    ///< Hover highlight color for this mesh
    RenderColor m_selectedColor{}; ///< Selected/picked highlight color for this mesh

    /**
     * @brief Check if mesh has valid data
     * @return true if mesh has vertices
     */
    [[nodiscard]] bool isValid() const { return !m_vertices.empty(); }

    /**
     * @brief Get vertex count
     * @return Number of vertices
     */
    [[nodiscard]] size_t vertexCount() const { return m_vertices.size(); }

    /**
     * @brief Get index count
     * @return Number of indices (0 for non-indexed)
     */
    [[nodiscard]] size_t indexCount() const { return m_indices.size(); }

    /**
     * @brief Check if mesh uses indexed rendering
     * @return true if index buffer is non-empty
     */
    [[nodiscard]] bool isIndexed() const { return !m_indices.empty(); }
};

struct GeometryRenderData : public BaseRenderData {
    Geometry::EntityId m_entityId{Geometry::INVALID_ENTITY_ID}; ///< Source entity ID
    Geometry::EntityUID m_entityUid{
        Geometry::INVALID_ENTITY_UID}; ///< Type-scoped UID (for picking)

    Geometry::EntityKey m_owningPart{};    ///< Owning part entity (if applicable)
    Geometry::EntityKey m_owningSolid{};   ///< Owning solid entity (if applicable)
    Geometry::EntityKeySet m_owningWire{}; ///< Owning wire entity (if applicable)
};

struct MeshRenderData : public BaseRenderData {
    Mesh::MeshNodeId m_nodeId{Mesh::INVALID_MESH_NODE_ID}; ///< Source node ID (for node meshes)

    Mesh::MeshElementId m_elementId{
        Mesh::INVALID_MESH_ELEMENT_ID}; ///< Source element ID (for element meshes)
    Mesh::MeshElementUID m_elementUid{
        Mesh::INVALID_MESH_ELEMENT_UID}; ///< Type-scoped UID (for picking)
};

struct DocumentRenderData {
    std::vector<GeometryRenderData> m_faceMeshes;   ///< Face/surface meshes
    std::vector<GeometryRenderData> m_edgeMeshes;   ///< Edge/curve meshes
    std::vector<GeometryRenderData> m_vertexMeshes; ///< Vertex/point meshes

    std::vector<MeshRenderData> m_meshElementMeshes; ///< FEM mesh element wireframe meshes
    std::vector<MeshRenderData> m_meshNodeMeshes;    ///< FEM mesh node point meshes

    Geometry::BoundingBox3D m_boundingBox; ///< Combined bounding box

    uint64_t m_version{0}; ///< Data version for change detection

    void updateGeometryRenderData(const DocumentRenderData& new_data) {
        m_faceMeshes = new_data.m_faceMeshes;
        m_edgeMeshes = new_data.m_edgeMeshes;
        m_vertexMeshes = new_data.m_vertexMeshes;
        m_boundingBox.expand(new_data.m_boundingBox);

        markModified();
    }

    void updateMeshRenderData(const DocumentRenderData& new_data) {
        m_meshElementMeshes = new_data.m_meshElementMeshes;
        m_meshNodeMeshes = new_data.m_meshNodeMeshes;
        m_boundingBox.expand(new_data.m_boundingBox);

        markModified();
    }

    /**
     * @brief Check if render data is empty
     * @return true if no meshes are present
     */
    [[nodiscard]] bool isEmpty() const {
        return m_faceMeshes.empty() && m_edgeMeshes.empty() && m_vertexMeshes.empty() &&
               m_meshElementMeshes.empty() && m_meshNodeMeshes.empty();
    }

    /**
     * @brief Get total mesh count
     * @return Sum of all mesh counts
     */
    [[nodiscard]] size_t meshCount() const {
        return m_faceMeshes.size() + m_edgeMeshes.size() + m_vertexMeshes.size() +
               m_meshElementMeshes.size() + m_meshNodeMeshes.size();
    }

    /**
     * @brief Clear all render data
     */
    void clear() {
        m_faceMeshes.clear();
        m_edgeMeshes.clear();
        m_vertexMeshes.clear();
        m_meshElementMeshes.clear();
        m_meshNodeMeshes.clear();
        m_boundingBox = Geometry::BoundingBox3D();
        ++m_version;
    }

    /**
     * @brief Increment version to signal data change
     */
    void markModified() { ++m_version; }

    /**
     * @brief Update combined bounding box from all meshes
     */
    void updateBoundingBox() {
        m_boundingBox = Geometry::BoundingBox3D();
        for(const auto& mesh : m_faceMeshes) {
            m_boundingBox.expand(mesh.m_boundingBox);
        }
        for(const auto& mesh : m_edgeMeshes) {
            m_boundingBox.expand(mesh.m_boundingBox);
        }
        for(const auto& mesh : m_vertexMeshes) {
            m_boundingBox.expand(mesh.m_boundingBox);
        }
        for(const auto& mesh : m_meshElementMeshes) {
            m_boundingBox.expand(mesh.m_boundingBox);
        }
        for(const auto& mesh : m_meshNodeMeshes) {
            m_boundingBox.expand(mesh.m_boundingBox);
        }
    }
};

/**
 * @brief Options for tessellation and mesh generation
 */
struct TessellationOptions {
    double m_linearDeflection{0.1};  ///< Linear deflection for surface tessellation
    double m_angularDeflection{0.5}; ///< Angular deflection in radians
    bool m_computeNormals{true};     ///< Compute vertex normals

    /**
     * @brief Create default options suitable for visualization
     * @return TessellationOptions with balanced quality/performance
     */
    [[nodiscard]] static TessellationOptions defaultOptions() {
        return TessellationOptions{0.05, 0.25, true};
    }

    /**
     * @brief Create high-quality options for detailed rendering
     * @return TessellationOptions with higher quality
     */
    [[nodiscard]] static TessellationOptions highQuality() {
        return TessellationOptions{0.01, 0.1, true};
    }

    /**
     * @brief Create low-quality options for fast preview
     * @return TessellationOptions with lower quality
     */
    [[nodiscard]] static TessellationOptions fastPreview() {
        return TessellationOptions{0.1, 0.5, false};
    }
};

} // namespace OpenGeoLab::Render
