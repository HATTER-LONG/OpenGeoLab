/**
 * @file pick_fbo.hpp
 * @brief Offscreen FBO for GPU entity picking
 */

#pragma once

#include <QOpenGLFunctions>
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Off-screen framebuffer object used for GPU picking.
 *
 * The FBO carries a single RG32UI color attachment (stores a uint64_t
 * pick-id split into two 32-bit channels) and a depth renderbuffer
 * (DEPTH_COMPONENT24).
 */
class PickFbo {
public:
    PickFbo() = default;
    ~PickFbo();

    PickFbo(const PickFbo&) = delete;
    PickFbo& operator=(const PickFbo&) = delete;

    /**
     * @brief Create the FBO with the given dimensions.
     * @return true on success
     */
    bool initialize(int w, int h);

    /**
     * @brief Recreate attachments for a new size.
     * @return true on success
     */
    bool resize(int w, int h);

    /** @brief Delete all GL objects and reset state */
    void cleanup();

    /** @brief Bind this FBO as the current render target */
    void bind();

    /** @brief Unbind (restore default framebuffer) */
    void unbind();

    /**
     * @brief Read the pick-id at pixel (x, y).
     *
     * Reads two GL_UNSIGNED_INT values from the RG32UI attachment and
     * reassembles them into a single uint64_t:  (high << 32) | low.
     *
     * @return Encoded pick id, or 0 if nothing was hit
     */
    [[nodiscard]] uint64_t readPickId(int x, int y) const;

    /**
     * @brief Read pick-ids in a square region around (cx, cy).
     *
     * Reads a (2*radius+1)^2 pixel region, reassembles each pixel into
     * a uint64_t pick id, and returns all non-zero (hit) ids.
     *
     * @param cx Center x in framebuffer coordinates
     * @param cy Center y in framebuffer coordinates
     * @param radius Half-size of the region (e.g. 3 → 7x7 pixels)
     * @return Vector of non-zero encoded pick ids
     */
    [[nodiscard]] std::vector<uint64_t> readPickRegion(int cx, int cy, int radius) const;

    // ── Accessors ────────────────────────────────────────────────────────

    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    GLuint m_fbo{0};
    GLuint m_colorTex{0};
    GLuint m_depthRbo{0};
    int m_width{0};
    int m_height{0};
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render
