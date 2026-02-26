#pragma once

#include "geometry/geometry_types.hpp"
#include "render/render_types.hpp"

#include <cstdint>
#include <unordered_map>
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

enum class RenderDisplayModeMask : uint8_t {
    None = 0,
    Surface = 1 << 0,
    Wireframe = 1 << 1,
    Points = 1 << 2,
    Mesh = 1 << 3
};

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

    [[nodiscard]] static constexpr uint64_t decodeUID(uint64_t encoded) {
        return encoded >> 8u;
    }

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
    uint64_t m_pickId{0};                      ///< Encoded pick ID (8 bytes)
};

// =============================================================================
// DrawRange — Describes one draw call into the flat buffer
// =============================================================================

/**
 * @brief A contiguous range within a RenderPassData buffer for a single draw call.
 */
struct DrawRange {
    uint32_t m_vertexOffset{0}; ///< First vertex index in the pass vertex buffer
    uint32_t m_vertexCount{0};  ///< Number of vertices
    uint32_t m_indexOffset{0};  ///< First index in the pass index buffer (0 = non-indexed)
    uint32_t m_indexCount{0};   ///< Number of indices (0 = non-indexed draw)
    PrimitiveTopology m_topology{PrimitiveTopology::Triangles};
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

struct RenderNodeKeyHash {
    std::size_t operator()(const RenderNodeKey& k) const noexcept {
        std::size_t h = static_cast<std::size_t>(k.m_type);
        h ^= std::hash<uint64_t>{}(k.m_uid) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};

// =============================================================================
// RenderNode — Semantic tree node
// =============================================================================

/**
 * @brief A node in the render semantic tree.
 *
 * Mirrors the geometry/mesh entity hierarchy. Each node carries metadata
 * (key, visibility, color, bounding box) and references to DrawRanges in
 * flat GPU buffers. The tree is used for scene management, visibility
 * toggling, and picking resolution — it does NOT own vertex data.
 */
struct RenderNode {
    RenderNodeKey m_key;            ///< Entity identity (type + uid)
    RenderColor m_color;            ///< Display color
    bool m_visible{true};           ///< Visibility flag
    Geometry::BoundingBox3D m_bbox; ///< Axis-aligned bounding box

    /// DrawRanges per pass (e.g., a Face node has Triangles in Geometry pass)
    std::unordered_map<RenderPassType, std::vector<DrawRange>> m_drawRanges;

    /// Child nodes (Part -> Solid -> Face/Edge/Vertex hierarchy)
    std::vector<RenderNode> m_children;
};

// =============================================================================
// RenderPassData — Flat GPU buffer for a single render pass
// =============================================================================

/**
 * @brief Aggregated vertex/index data for one render pass.
 *
 * All geometry (or mesh) primitives of the same pass are packed into a
 * single contiguous buffer to minimize GPU state changes and draw calls.
 */
struct RenderPassData {
    std::vector<RenderVertex> m_vertices; ///< Flat vertex buffer
    std::vector<uint32_t> m_indices;      ///< Flat index buffer
    bool m_dirty{true};                   ///< True when buffers need GPU re-upload
};

// =============================================================================
// RenderData — Top-level render data container
// =============================================================================

/**
 * @brief Complete render data snapshot combining semantic tree and flat GPU buffers.
 *
 * Produced by GeometryDocument and MeshDocument, consumed by the GL renderer.
 */
struct RenderData {
    /// Semantic tree roots (one per top-level Part or mesh group)
    std::vector<RenderNode> m_roots;

    /// Per-pass flat GPU buffer data
    std::unordered_map<RenderPassType, RenderPassData> m_passData;

    /// Scene-wide bounding box (union of all visible geometry)
    Geometry::BoundingBox3D m_sceneBBox;

    /// Dirty flags per domain
    bool m_geometryDirty{true};
    bool m_meshDirty{true};

    /**
     * @brief Clear all render data (geometry + mesh)
     */
    void clear() {
        m_roots.clear();
        m_passData.clear();
        m_sceneBBox = {};
        m_geometryDirty = true;
        m_meshDirty = true;
    }

    /**
     * @brief Clear geometry-domain data only, preserving mesh data
     */
    void clearGeometry() {
        // Remove geometry roots (RenderEntityType in geometry domain)
        std::erase_if(m_roots,
                      [](const RenderNode& n) { return isGeometryDomain(n.m_key.m_type); });
        // Clear geometry pass buffer
        m_passData.erase(RenderPassType::Geometry);
        m_geometryDirty = true;
    }

    /**
     * @brief Clear mesh-domain data only, preserving geometry data
     */
    void clearMesh() {
        std::erase_if(m_roots, [](const RenderNode& n) { return isMeshDomain(n.m_key.m_type); });
        m_passData.erase(RenderPassType::Mesh);
        m_meshDirty = true;
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
