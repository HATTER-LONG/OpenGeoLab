/**
 * @file pick_fbo.hpp
 * @brief Off-screen framebuffer for GPU-based entity picking.
 *
 * Uses a GL_RG32UI color attachment to store 64-bit PickId values
 * (split as two uint32 components) alongside a depth buffer for
 * correct occlusion during the pick render pass.
 */

#pragma once

#include <QOpenGLExtraFunctions>
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief A single pixel read from the pick FBO.
 *
 * Contains the two uint32 components of a PickId. Component layout
 * matches the RenderVertex::m_pickId split (little-endian uint64_t):
 *   m_lo = lower 32 bits (contains entity type in low 8 bits)
 *   m_hi = upper 32 bits
 */
struct PickPixel {
    uint32_t m_lo{0}; ///< Lower 32 bits of pickId
    uint32_t m_hi{0}; ///< Upper 32 bits of pickId

    /** @brief Reconstruct the full 64-bit pickId. */
    [[nodiscard]] uint64_t pickId() const {
        return (static_cast<uint64_t>(m_hi) << 32) | static_cast<uint64_t>(m_lo);
    }

    /** @brief True if this pixel is the background (no entity). */
    [[nodiscard]] bool isBackground() const { return m_lo == 0 && m_hi == 0; }
};

/**
 * @brief Off-screen FBO with GL_RG32UI color attachment for GPU picking.
 *
 * Thread-safety: requires GL context, NOT thread-safe.
 * All operations must occur on the GL rendering thread.
 */
class PickFBO {
public:
    PickFBO() = default;
    ~PickFBO();

    PickFBO(const PickFBO&) = delete;
    PickFBO& operator=(const PickFBO&) = delete;

    /** @brief Create FBO with the given dimensions. */
    void initialize(int width, int height);

    /** @brief Recreate attachments for a new viewport size. */
    void resize(int width, int height);

    /** @brief Release all GL resources. */
    void cleanup();

    /** @brief Bind this FBO for rendering and clear it. */
    void bind();

    /** @brief Unbind this FBO (restore default framebuffer 0). */
    void unbind();

    /**
     * @brief Read a rectangular region of pick pixels.
     * @param x Left edge in pixels
     * @param y Bottom edge in pixels
     * @param w Width in pixels
     * @param h Height in pixels
     * @return Vector of PickPixel values (row-major, size = w * h).
     */
    [[nodiscard]] std::vector<PickPixel> readRegion(int x, int y, int w, int h) const;

    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    void createAttachments();
    void destroyAttachments();

    GLuint m_fbo{0};
    GLuint m_colorTex{0}; ///< GL_RG32UI texture
    GLuint m_depthRbo{0}; ///< GL_DEPTH_COMPONENT24 renderbuffer
    int m_width{0};
    int m_height{0};
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
