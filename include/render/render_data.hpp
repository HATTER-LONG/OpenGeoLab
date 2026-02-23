/**
 * @file render_data.hpp
 * @brief Render data structures for OpenGL geometry visualization
 *
 * Defines data structures for transferring geometry to the rendering layer.
 * Uses a batched design: one RenderMesh per entity category (faces, edges, etc.)
 * with per-entity metadata in RenderEntityInfoMap for selection/hover sub-draw.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "render/render_types.hpp"

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
 * @brief Vertex data for rendering with position, normal, color, and entity uid.
 *
 * Packed structure for efficient GPU upload. The uid field carries the
 * packed RenderUID so the pick shader can identify entities per-fragment
 * without per-entity uniform calls.
 *
 * Layout: position (3 floats), normal (3 floats), color (4 floats), uid (2 uint32)
 * Total: 48 bytes per vertex.
 */
struct RenderVertex {
    float m_position[3]{0.0f, 0.0f, 0.0f};       ///< Vertex position (x, y, z)
    float m_normal[3]{0.0f, 0.0f, 1.0f};         ///< Vertex normal for lighting
    RenderColor m_color{0.8f, 0.8f, 0.8f, 1.0f}; ///< RGBA color
    uint32_t m_uidLow{0};                        ///< Low 32 bits of packed RenderUID
    uint32_t m_uidHigh{0};                       ///< High 32 bits of packed RenderUID

    void setUid(uint64_t packed) {
        m_uidLow = static_cast<uint32_t>(packed & 0xFFFFFFFFu);
        m_uidHigh = static_cast<uint32_t>((packed >> 32u) & 0xFFFFFFFFu);
    }
    [[nodiscard]] uint64_t uid() const {
        return static_cast<uint64_t>(m_uidLow) | (static_cast<uint64_t>(m_uidHigh) << 32u);
    }

    RenderVertex() = default;

    RenderVertex(float x, float y, float z) {
        m_position[0] = x;
        m_position[1] = y;
        m_position[2] = z;
    }

    RenderVertex(float px, float py, float pz, float nx, float ny, float nz) { // NOLINT
        m_position[0] = px;
        m_position[1] = py;
        m_position[2] = pz;
        m_normal[0] = nx;
        m_normal[1] = ny;
        m_normal[2] = nz;
    }

    void setColor(float r, float g, float b, float a = 1.0f) {
        m_color.m_r = r;
        m_color.m_g = g;
        m_color.m_b = b;
        m_color.m_a = a;
    }
};

static_assert(sizeof(RenderVertex) == 48, "RenderVertex must be 48 bytes for GPU layout");

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

/**
 * @brief Batched mesh data container
 *
 * Holds vertex and index data for a category of entities (all faces, all edges, etc.).
 * Per-entity identity is encoded in each vertex's m_uid field.
 * Entity metadata (ownership, ranges, colors) is stored externally in RenderEntityInfoMap.
 */
struct RenderMesh {
    RenderPrimitiveType m_primitiveType{RenderPrimitiveType::Triangles}; ///< Primitive type

    std::vector<RenderVertex> m_vertices; ///< Vertex data
    std::vector<uint32_t> m_indices;      ///< Index data (empty for non-indexed draw)

    Geometry::BoundingBox3D m_boundingBox; ///< Mesh bounding box

    [[nodiscard]] bool isValid() const { return !m_vertices.empty(); }
    [[nodiscard]] size_t vertexCount() const { return m_vertices.size(); }
    [[nodiscard]] size_t indexCount() const { return m_indices.size(); }
    [[nodiscard]] bool isIndexed() const { return !m_indices.empty(); }
};

/**
 * @brief Complete render data for a document (geometry + mesh)
 *
 * Contains batched mesh data organized by entity category. Each category
 * has a single RenderMesh (batch) plus a RenderEntityInfoMap for per-entity
 * metadata needed for selection highlighting and sub-draw.
 */
struct DocumentRenderData {
    RenderMesh m_faceBatch;        ///< All faces (GL_TRIANGLES, indexed)
    RenderMesh m_edgeBatch;        ///< All edges (GL_LINES, indexed)
    RenderMesh m_vertexBatch;      ///< All vertices (GL_POINTS)
    RenderMesh m_meshElementBatch; ///< All FEM elements (GL_LINES, indexed)
    RenderMesh m_meshNodeBatch;    ///< All FEM nodes (GL_POINTS)

    RenderEntityInfoMap m_faceEntities;
    RenderEntityInfoMap m_edgeEntities;
    RenderEntityInfoMap m_vertexEntities;
    RenderEntityInfoMap m_meshElementEntities;
    RenderEntityInfoMap m_meshNodeEntities;

    Geometry::BoundingBox3D m_boundingBox; ///< Combined bounding box
    uint64_t m_version{0};                 ///< Data version for change detection

    [[nodiscard]] bool isEmpty() const {
        return !m_faceBatch.isValid() && !m_edgeBatch.isValid() && !m_vertexBatch.isValid() &&
               !m_meshElementBatch.isValid() && !m_meshNodeBatch.isValid();
    }

    [[nodiscard]] size_t entityCount() const {
        return m_faceEntities.size() + m_edgeEntities.size() + m_vertexEntities.size() +
               m_meshElementEntities.size() + m_meshNodeEntities.size();
    }

    /** @brief Clear all batches and entity maps, increment data version. */
    void clear() {
        m_faceBatch = RenderMesh();
        m_edgeBatch = RenderMesh();
        m_vertexBatch = RenderMesh();
        m_meshElementBatch = RenderMesh();
        m_meshNodeBatch = RenderMesh();
        m_faceEntities.clear();
        m_edgeEntities.clear();
        m_vertexEntities.clear();
        m_meshElementEntities.clear();
        m_meshNodeEntities.clear();
        m_boundingBox = Geometry::BoundingBox3D();
        ++m_version;
    }

    void markModified() { ++m_version; }

    /** @brief Recompute combined bounding box from all sub-batch bounding boxes. */
    void updateBoundingBox() {
        m_boundingBox = Geometry::BoundingBox3D();
        m_boundingBox.expand(m_faceBatch.m_boundingBox);
        m_boundingBox.expand(m_edgeBatch.m_boundingBox);
        m_boundingBox.expand(m_vertexBatch.m_boundingBox);
        m_boundingBox.expand(m_meshElementBatch.m_boundingBox);
        m_boundingBox.expand(m_meshNodeBatch.m_boundingBox);
    }

    /**
     * @brief Merge another DocumentRenderData into this one (move semantics).
     *
     * Appends the other's batched data to this document's batches,
     * adjusting index offsets for correct rendering.
     */
    void merge(DocumentRenderData&& other) {
        mergeBatch(m_faceBatch, m_faceEntities, std::move(other.m_faceBatch),
                   std::move(other.m_faceEntities));
        mergeBatch(m_edgeBatch, m_edgeEntities, std::move(other.m_edgeBatch),
                   std::move(other.m_edgeEntities));
        mergeBatch(m_vertexBatch, m_vertexEntities, std::move(other.m_vertexBatch),
                   std::move(other.m_vertexEntities));
        mergeBatch(m_meshElementBatch, m_meshElementEntities, std::move(other.m_meshElementBatch),
                   std::move(other.m_meshElementEntities));
        mergeBatch(m_meshNodeBatch, m_meshNodeEntities, std::move(other.m_meshNodeBatch),
                   std::move(other.m_meshNodeEntities));
    }

private:
    static void mergeBatch(RenderMesh& dst,
                           RenderEntityInfoMap& dst_entities,
                           RenderMesh&& src,
                           RenderEntityInfoMap&& src_entities) {
        if(!src.isValid()) {
            return;
        }
        if(!dst.isValid()) {
            dst = std::move(src);
            dst_entities.merge(std::move(src_entities));
            return;
        }
        const auto base_vertex = static_cast<uint32_t>(dst.m_vertices.size());
        const auto base_index = static_cast<uint32_t>(dst.m_indices.size());

        dst.m_vertices.insert(dst.m_vertices.end(), std::make_move_iterator(src.m_vertices.begin()),
                              std::make_move_iterator(src.m_vertices.end()));

        dst.m_indices.reserve(dst.m_indices.size() + src.m_indices.size());
        for(uint32_t idx : src.m_indices) {
            dst.m_indices.push_back(idx + base_vertex);
        }

        for(auto& [key, info] : src_entities) {
            info.m_indexOffset += base_index;
            info.m_vertexOffset += base_vertex;
            dst_entities[key] = std::move(info);
        }

        dst.m_boundingBox.expand(src.m_boundingBox);
    }
};

/**
 * @brief Options for tessellation and mesh generation
 */
struct TessellationOptions {
    double m_linearDeflection{0.1};  ///< Linear deflection for surface tessellation
    double m_angularDeflection{0.5}; ///< Angular deflection in radians
    bool m_computeNormals{true};     ///< Compute vertex normals

    [[nodiscard]] static TessellationOptions defaultOptions() {
        return TessellationOptions{0.05, 0.25, true};
    }

    [[nodiscard]] static TessellationOptions highQuality() {
        return TessellationOptions{0.01, 0.1, true};
    }

    [[nodiscard]] static TessellationOptions fastPreview() {
        return TessellationOptions{0.1, 0.5, false};
    }
};

} // namespace OpenGeoLab::Render
