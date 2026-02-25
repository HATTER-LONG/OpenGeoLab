#pragma once

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
    Geometry = 0,
    Mesh = 1,
    Post = 2,
};

/**
 * @brief Viewport display mode for first-phase rendering
 *
 * In-scope for phase-1:
 * - Surface (triangles)
 * - Wireframe (lines)
 * - Points
 *
 * Out-of-scope for phase-1:
 * - Texture/material/PBR shading
 * - Advanced post-processing effects
 */
enum class RenderDisplayMode : uint8_t {
    Surface = 0,
    Wireframe = 1,
    Points = 2,
};

/**
 * @brief CPU-side primitive payload for one draw item
 */
struct RenderPrimitive {
    RenderPassType m_pass{RenderPassType::Geometry};       ///< Target pass
    RenderEntityType m_entityType{RenderEntityType::None}; ///< Source entity classification
    uint64_t m_entityUID{0};                               ///< Type-scoped entity uid for picking
    PrimitiveTopology m_topology{PrimitiveTopology::Triangles}; ///< Primitive topology
    RenderColor m_color{};                                      ///< Base color
    bool m_visible{true};                                       ///< Visibility flag

    std::vector<float> m_positions;  ///< xyz triplets
    std::vector<uint32_t> m_indices; ///< Optional indexed topology

    [[nodiscard]] bool empty() const { return m_positions.empty(); }
};

/**
 * @brief Collection of primitives generated from one document domain
 */
struct RenderData {
    std::vector<RenderPrimitive> m_primitives;

    void clear() { m_primitives.clear(); }

    [[nodiscard]] bool empty() const { return m_primitives.empty(); }

    [[nodiscard]] size_t primitiveCount() const { return m_primitives.size(); }
};

/**
 * @brief Per-frame render bucket grouped by pass
 */
struct RenderBucket {
    RenderData m_geometryPass;
    RenderData m_meshPass;
    RenderData m_postPass;

    void clear() {
        m_geometryPass.clear();
        m_meshPass.clear();
        m_postPass.clear();
    }

    [[nodiscard]] bool empty() const {
        return m_geometryPass.empty() && m_meshPass.empty() && m_postPass.empty();
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
