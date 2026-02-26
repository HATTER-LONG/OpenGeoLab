#pragma once

#include "render/render_types.hpp"
#include <cstdint>
namespace OpenGeoLab::Render {

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

struct RenderPrimitive {
    uint64_t m_uid{0};
    uint64_t m_partUID{0};

    RenderEntityType m_entityType{RenderEntityType::None};
    PrimitiveTopology m_topology{PrimitiveTopology::Triangles};
    RenderPassType m_passType{RenderPassType::None};

    bool m_visible{true};

    std::vector<Util::Pt3d> m_positions;
    std::vector<uint32_t> m_indices;

    [[nodiscard]] bool isValid() const {
        if(m_positions.empty() || m_uid == 0 || m_entityType == RenderEntityType::None ||
           m_passType == RenderPassType::None) {
            return false;
        }

        if(m_topology == PrimitiveTopology::Points) {
            return true;
        }

        return !m_indices.empty();
    }
};

struct RenderData {
    std::vector<RenderPrimitive> m_geometry;
    std::vector<RenderPrimitive> m_mesh;
    std::vector<RenderPrimitive> m_post;

    void clear() {
        m_geometry.clear();
        m_mesh.clear();
        m_post.clear();
    }

    bool hasContent() const { return !m_geometry.empty() || !m_mesh.empty() || !m_post.empty(); }
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
