/**
 * @file render_mesh_data.hpp
 * @brief GPU-oriented mesh data for scene rendering
 *
 * RenderVertex packs position, normal, and color into a 40-byte vertex.
 * PickIdEntry holds the 64-bit pick identifier per vertex (8 bytes).
 * DrawRange describes a contiguous sub-mesh for backend draw calls.
 * RenderMeshData aggregates all per-shape render data.
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/pick_id.hpp>

#include <opengeolab/core/entity_tag.hpp>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Scene {

/**
 * @brief GPU vertex: position + normal + RGBA color = 40 bytes.
 *
 * Two-VBO layout:
 *   Main VBO: array of RenderVertex (40B each)
 *   Pick VBO: array of PickIdEntry (8B each)
 * Vertex count must be identical between the two arrays.
 */
struct RenderVertex {
    float position[3]{}; /**< World-space position */
    float normal[3]{};   /**< Unit normal */
    float color[4]{};    /**< RGBA color */
};
static_assert(sizeof(RenderVertex) == 40, "RenderVertex must be 40 bytes for GPU alignment");

/**
 * @brief Per-vertex pick identifier for the pick VBO.
 *
 * The pick pass reads pickId to identify which sub-entity was hit.
 */
struct PickIdEntry {
    uint64_t pickId{0}; /**< Encoded via PickId::encode() */
};
static_assert(sizeof(PickIdEntry) == 8, "PickIdEntry must be 8 bytes");

/** @brief Topology type of a draw range. */
enum class PrimitiveTopology : uint8_t {
    Triangles, /**< GL_TRIANGLES */
    Lines,     /**< GL_LINES */
    Points,    /**< GL_POINTS */
};

/**
 * @brief Describes a contiguous sub-mesh for a single entity.
 *
 * Renderer uses DrawRange to issue indexed or non-indexed draw commands.
 * per entity. Also used for highlight expansion (draw all edges of
 * a selected wire, all faces of a solid, etc.).
 */
struct DrawRange {
    uint32_t shapeId{};            /**< Owning shape */
    Core::EntityType entityType{}; /**< Type of this sub-entity */
    uint32_t localId{};            /**< Sub-entity local index */
    uint32_t vertexOffset{};       /**< First vertex in the main VBO */
    uint32_t vertexCount{};        /**< Number of vertices */
    uint32_t indexOffset{};        /**< First index in the IBO */
    uint32_t indexCount{};         /**< Number of indices */
    PrimitiveTopology topology{};  /**< Primitive type */
};

/**
 * @brief Complete render data for one shape.
 *
 * Aggregates all vertices, pick IDs, indices, and draw ranges
 * partitioned by primitive topology.
 */
struct RenderMeshData {
    std::vector<RenderVertex> vertices;    /**< Main vertex data */
    std::vector<PickIdEntry> pickIds;      /**< Per-vertex pick IDs (same count as vertices) */
    std::vector<uint32_t> indices;         /**< Index buffer */
    std::vector<DrawRange> triangleRanges; /**< Face draw ranges */
    std::vector<DrawRange> lineRanges;     /**< Edge draw ranges */
    std::vector<DrawRange> pointRanges;    /**< Vertex draw ranges */
    BoundingBox3D bounds;                  /**< AABB of all vertices */
    uint64_t version{0};                   /**< Monotonic dirty counter */

    /** @brief Increment version to signal data change. */
    void markUpdated() { ++version; }
};

} // namespace OpenGeoLab::Scene
