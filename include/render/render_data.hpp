#pragma once

#include "render/render_types.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
namespace OpenGeoLab::Render {

// =============================================================================
// RenderColor
// =============================================================================

/**
 * @brief Simple RGBA color used by the render layer
 */
struct RenderColor {
    float m_r{0.8f}; ///< Red component [0, 1]
    float m_g{0.8f}; ///< Green component [0, 1]
    float m_b{0.8f}; ///< Blue component [0, 1]
    float m_a{1.0f}; ///< Alpha component [0, 1]

    [[nodiscard]] std::string toHex() const {
        auto to_byte = [](float c) -> uint8_t {
            return static_cast<uint8_t>(std::round(c * 255.0f));
        };
        char buffer[9];
        std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", to_byte(m_r), to_byte(m_g),
                      to_byte(m_b));
        return std::string(buffer);
    }
};

// =============================================================================
// Enums
// =============================================================================

/**
 * @brief Primitive topology used for draw calls
 */
enum class PrimitiveTopology : uint8_t {
    Points = 0,
    Lines = 1,
    Triangles = 2,
};

/**
 * @brief Render pass category
 */
enum class RenderPassType : uint8_t {
    None = 0,
    Geometry = 1,
    Mesh = 2,
    Post = 3,
};

/**
 * @brief Bitmask controlling which display modes are active.
 */
enum class RenderDisplayModeMask : uint8_t {
    None = 0,
    Surface = 1 << 0,   ///< Shaded surface rendering
    Wireframe = 1 << 1, ///< Wireframe overlay
    Points = 1 << 2,    ///< Point cloud rendering
    Mesh = 1 << 3       ///< Mesh element rendering
};

constexpr RenderDisplayModeMask operator|(RenderDisplayModeMask a, RenderDisplayModeMask b) {
    return static_cast<RenderDisplayModeMask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr RenderDisplayModeMask operator&(RenderDisplayModeMask a, RenderDisplayModeMask b) {
    return static_cast<RenderDisplayModeMask>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

// =============================================================================
// PickId — Encoded entity identifier for GPU picking
// =============================================================================
/**
 * @brief Encodes (RenderEntityType, UID) into a single uint64_t for GPU picking.
 *
 * Layout: [56-bit UID | 8-bit type]
 * Supports up to 2^56 unique entities per type. Encoded value 0 = background (no pick).
 */
struct PickId {
    uint64_t m_encoded{0};

    PickId() = default;
    explicit constexpr PickId(uint64_t encoded) : m_encoded(encoded) {}

    [[nodiscard]] static constexpr uint64_t encode(RenderEntityType type, uint64_t uid) {
        return (uid << 8u) | static_cast<uint64_t>(type);
    }

    [[nodiscard]] static constexpr RenderEntityType decodeType(uint64_t encoded) {
        return static_cast<RenderEntityType>(encoded & 0xFFu);
    }

    [[nodiscard]] static constexpr uint64_t decodeUID(uint64_t encoded) { return encoded >> 8u; }

    [[nodiscard]] constexpr bool isValid() const { return m_encoded != 0; }
};

// =============================================================================
// RenderVertex — Per-vertex data for GPU buffers
// =============================================================================

/**
 * @brief GPU vertex layout: position + normal + color + pickId
 *
 * Total stride: 48 bytes. Normals are zero for line/point primitives.
 * pickId is uint64_t [56-bit UID | 8-bit type], passed to GPU as uvec2.
 */
struct RenderVertex {
    float m_position[3]{0.0f, 0.0f, 0.0f};    ///< World-space position (12 bytes)
    float m_normal[3]{0.0f, 0.0f, 0.0f};      ///< Surface normal (12 bytes)
    float m_color[4]{0.8f, 0.8f, 0.8f, 1.0f}; ///< RGBA color (16 bytes)
    uint64_t m_pickId{0};                     ///< Encoded pick ID (8 bytes)
};
// =============================================================================
// RenderNodeKey — Identity for a semantic tree node
// =============================================================================

/**
 * @brief Identifies a RenderNode by (type, uid), matching the entity ID system.
 */
struct RenderNodeKey {
    RenderEntityType m_type{RenderEntityType::None};
    uint64_t m_uid{0};

    bool operator==(const RenderNodeKey& o) const { return m_type == o.m_type && m_uid == o.m_uid; }
    bool operator!=(const RenderNodeKey& o) const { return !(*this == o); }
};

/** @brief Hash functor for RenderNodeKey, suitable for unordered containers. */
struct RenderNodeKeyHash {
    std::size_t operator()(const RenderNodeKey& k) const noexcept {
        std::size_t h = static_cast<std::size_t>(k.m_type);
        h ^= std::hash<uint64_t>{}(k.m_uid) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};

// =============================================================================
// DrawRange — Describes one draw call into the flat buffer
// =============================================================================

/**
 * @brief A contiguous range within a RenderPassData buffer for a single draw call.
 */
struct DrawRange {
    RenderNodeKey m_entityKey; ///< Entity identity (type + uid)
    uint64_t m_partUid{0};     ///< Parent part uid for reverse lookup
    uint64_t m_solidUid{0};    ///< Parent solid uid for aggregate highlight / lookup
    uint64_t m_wireUid{0};     ///< Parent wire uid for edge-to-wire lookup

    uint32_t m_vertexOffset{0}; ///< First vertex index in the pass vertex buffer
    uint32_t m_vertexCount{0};  ///< Number of vertices
    uint32_t m_indexOffset{0};  ///< First index in the pass index buffer (0 = non-indexed)
    uint32_t m_indexCount{0};   ///< Number of indices (0 = non-indexed draw)
    PrimitiveTopology m_topology{PrimitiveTopology::Triangles};
};

// =============================================================================
// Multi-draw batch caches
// =============================================================================

/**
 * @brief Indexed multi-draw batch built from DrawRange index spans.
 *
 * Stores byte offsets instead of raw pointers so batches remain CPU-side data
 * that can be cached inside RenderData and later converted to GL pointers at
 * draw time.
 */
struct IndexedDrawBatch {
    std::vector<int32_t> m_counts;
    std::vector<uintptr_t> m_byteOffsets;

    [[nodiscard]] bool empty() const { return m_counts.empty(); }

    void clear() {
        m_counts.clear();
        m_byteOffsets.clear();
    }

    void reserve(size_t size) {
        m_counts.reserve(size);
        m_byteOffsets.reserve(size);
    }

    void append(const DrawRange& range) {
        if(range.m_indexCount == 0) {
            return;
        }
        m_counts.push_back(static_cast<int32_t>(range.m_indexCount));
        m_byteOffsets.push_back(static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t));
    }

    void append(const IndexedDrawBatch& other) {
        m_counts.insert(m_counts.end(), other.m_counts.begin(), other.m_counts.end());
        m_byteOffsets.insert(m_byteOffsets.end(), other.m_byteOffsets.begin(),
                             other.m_byteOffsets.end());
    }
};

/**
 * @brief Non-indexed multi-draw batch built from DrawRange vertex spans.
 */
struct ArrayDrawBatch {
    std::vector<int32_t> m_firsts;
    std::vector<int32_t> m_counts;

    [[nodiscard]] bool empty() const { return m_counts.empty(); }

    void clear() {
        m_firsts.clear();
        m_counts.clear();
    }

    void reserve(size_t size) {
        m_firsts.reserve(size);
        m_counts.reserve(size);
    }

    void append(const DrawRange& range) {
        if(range.m_vertexCount == 0) {
            return;
        }
        m_firsts.push_back(static_cast<int32_t>(range.m_vertexOffset));
        m_counts.push_back(static_cast<int32_t>(range.m_vertexCount));
    }

    void append(const ArrayDrawBatch& other) {
        m_firsts.insert(m_firsts.end(), other.m_firsts.begin(), other.m_firsts.end());
        m_counts.insert(m_counts.end(), other.m_counts.begin(), other.m_counts.end());
    }
};

/**
 * @brief Cached indexed batches for a topology, both full-domain and per-entity-type.
 */
struct IndexedBatchCache {
    IndexedDrawBatch m_all;
    std::unordered_map<RenderEntityType, IndexedDrawBatch> m_byType;

    void reserve(size_t size) { m_all.reserve(size); }

    void append(const DrawRange& range) {
        m_all.append(range);
        m_byType[range.m_entityKey.m_type].append(range);
    }

    void clear() {
        m_all.clear();
        m_byType.clear();
    }
};

/**
 * @brief Cached array batches for a topology, both full-domain and per-entity-type.
 */
struct ArrayBatchCache {
    ArrayDrawBatch m_all;
    std::unordered_map<RenderEntityType, ArrayDrawBatch> m_byType;

    void reserve(size_t size) { m_all.reserve(size); }

    void append(const DrawRange& range) {
        m_all.append(range);
        m_byType[range.m_entityKey.m_type].append(range);
    }

    void clear() {
        m_all.clear();
        m_byType.clear();
    }
};

/**
 * @brief Cached multi-draw batches for CAD geometry ranges.
 */
struct GeometryDrawBatchCache {
    IndexedBatchCache m_triangles;
    IndexedBatchCache m_lines;
    ArrayBatchCache m_points;

    void clear() {
        m_triangles.clear();
        m_lines.clear();
        m_points.clear();
    }
};

/**
 * @brief Cached multi-draw batches for mesh ranges.
 */
struct MeshDrawBatchCache {
    ArrayBatchCache m_triangles;
    ArrayBatchCache m_lines;
    ArrayBatchCache m_points;

    void clear() {
        m_triangles.clear();
        m_lines.clear();
        m_points.clear();
    }
};

// =============================================================================
// RenderPassData — Flat GPU buffer for a single render pass
// =============================================================================

/**
 * @brief Aggregated vertex/index data for one render pass.
 *
 * All geometry (or mesh) primitives of the same pass are packed into a
 * single contiguous buffer to minimize GPU state changes and draw calls.
 *
 * Uses a version-based tracking system: data is re-uploaded to GPU only when
 * m_version differs from GpuBuffer's recorded upload version. This is more explicit
 * and efficient than boolean dirty flags.
 */
struct RenderPassData {
    std::vector<RenderVertex> m_vertices; ///< Flat vertex buffer
    std::vector<uint32_t> m_indices;      ///< Flat index buffer
    uint64_t m_version{0}; ///< Current data version number (incremented when data changes)

    /**
     * @brief Check if data needs to be uploaded to GPU.
     * Should call GpuBuffer::needsUpload(data) instead of this method.
     * @param uploaded_version Last version known to have been uploaded
     * @return true if m_version differs from uploaded_version
     */
    [[nodiscard]] bool needsUpload(uint64_t uploaded_version) const {
        return uploaded_version != m_version;
    }

    /**
     * @brief Mark data as updated (increment version).
     * Call this after modifying vertices or indices.
     */
    void markDataUpdated() { ++m_version; }
};

// =============================================================================
// PickResolutionData — Lookup tables for pick/hover entity resolution
// =============================================================================

/**
 * @brief Pick-specific lookup tables for resolving entity hierarchy during
 *        hover and click picking.
 *
 * These maps are built by GeometryRenderBuilder and MeshRenderBuilder and
 * consumed by RenderSceneImpl (processHover / processPicking) to resolve
 * edges to wires, wires to faces, and mesh line IDs to node pairs.
 * They are NOT used during normal rendering — only during pick resolution.
 */
struct PickResolutionData {
    /// Edge uid → parent wire uid(s) lookup (built by GeometryRenderBuilder).
    /// An edge shared between two faces belongs to two different wires.
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_edgeToWireUids;

    /// Edge uid → parent solid uid(s) lookup.
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_edgeToSolidUids;

    /// Wire uid → edge uids reverse lookup for complete wire highlighting
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_wireToEdgeUids;

    /// Wire uid → parent face uid lookup (each wire belongs to exactly one face)
    std::unordered_map<uint64_t, uint64_t> m_wireToFaceUid;

    /// Face uid → parent solid uid lookup.
    std::unordered_map<uint64_t, uint64_t> m_faceToSolidUid;

    /// Entity uid → parent part uid (built from DrawRangeEx data during build phase)
    std::unordered_map<uint64_t, uint64_t> m_entityToPartUid;

    /// Mesh line sequential ID → (nodeA, nodeB) lookup.
    /// Built by MeshRenderBuilder; used to resolve mesh line picks back to node pairs.
    std::unordered_map<uint64_t, std::pair<Mesh::MeshNodeId, Mesh::MeshNodeId>> m_meshLineNodes;

    void clear() {
        m_edgeToWireUids.clear();
        m_edgeToSolidUids.clear();
        m_wireToEdgeUids.clear();
        m_wireToFaceUid.clear();
        m_faceToSolidUid.clear();
        m_entityToPartUid.clear();
        m_meshLineNodes.clear();
    }

    void clearGeometry() {
        m_edgeToWireUids.clear();
        m_edgeToSolidUids.clear();
        m_wireToEdgeUids.clear();
        m_wireToFaceUid.clear();
        m_faceToSolidUid.clear();
        m_entityToPartUid.clear();
    }

    void clearMesh() { m_meshLineNodes.clear(); }
};

// =============================================================================
// RenderData — Top-level render data container
// =============================================================================

/**
 * @brief Complete render data snapshot combining semantic tree and flat GPU buffers.
 *
 * Produced by GeometryDocument and MeshDocument, consumed by the GL renderer.
 * Rendering data (roots, pass buffers, bounding box) and pick-specific lookup
 * tables (m_pickData) are separated for clarity.
 */
struct RenderData {
    /// Per-pass flat GPU buffer data
    std::unordered_map<RenderPassType, RenderPassData> m_passData;

    /// Pick-specific lookup tables for entity hierarchy resolution
    PickResolutionData m_pickData;

    /// Pre-built geometry DrawRangeEx by topology (generated during build phase)
    std::vector<DrawRange> m_geometryTriangleRanges;
    std::vector<DrawRange> m_geometryLineRanges;
    std::vector<DrawRange> m_geometryPointRanges;

    /// Pre-built mesh DrawRangeEx by topology (generated during build phase)
    std::vector<DrawRange> m_meshTriangleRanges;
    std::vector<DrawRange> m_meshLineRanges;
    std::vector<DrawRange> m_meshPointRanges;

    /// Cached multi-draw batches for geometry and mesh topologies
    GeometryDrawBatchCache m_geometryBatches;
    MeshDrawBatchCache m_meshBatches;

    /// Scene-wide bounding box (union of all visible geometry)
    Geometry::BoundingBox3D m_sceneBBox;

    /// Dirty flags per domain
    uint64_t m_geometryVersion{0};
    uint64_t m_meshVersion{0};

    void markGeometryUpdated() { ++m_geometryVersion; }
    void markMeshUpdated() { ++m_meshVersion; }

    /**
     * @brief Clear all render data (geometry + mesh)
     */
    void clear() {
        m_passData.clear();
        m_pickData.clear();
        m_geometryTriangleRanges.clear();
        m_geometryLineRanges.clear();
        m_geometryPointRanges.clear();
        m_meshTriangleRanges.clear();
        m_meshLineRanges.clear();
        m_meshPointRanges.clear();
        m_geometryBatches.clear();
        m_meshBatches.clear();
        m_sceneBBox = {};
    }

    /**
     * @brief Clear geometry-domain data only, preserving mesh data
     */
    void clearGeometry() {
        // Clear geometry pass buffer
        m_passData.erase(RenderPassType::Geometry);
        m_pickData.clearGeometry();
        m_geometryTriangleRanges.clear();
        m_geometryLineRanges.clear();
        m_geometryPointRanges.clear();
        m_geometryBatches.clear();
    }

    /**
     * @brief Clear mesh-domain data only, preserving geometry data
     */
    void clearMesh() {
        m_passData.erase(RenderPassType::Mesh);
        m_pickData.clearMesh();
        m_meshTriangleRanges.clear();
        m_meshLineRanges.clear();
        m_meshPointRanges.clear();
        m_meshBatches.clear();
    }
};

// =============================================================================
// TessellationOptions
// =============================================================================

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
