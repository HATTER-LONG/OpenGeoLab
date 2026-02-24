#pragma once

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

struct RenderData {};

struct RenderBucket {};

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
