/**
 * @file pick_fbo.hpp
 * @brief Off-screen FBO for GPU color picking (RG32UI + depth)
 */

#pragma once

#include <glad/gl.h>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

class PickFbo final {
public:
    bool initialize(int width, int height);
    void resize(int width, int height);
    void cleanup();

    void bind() const;
    void unbind() const;

    /** @brief Read a single pixel's pickId. */
    [[nodiscard]] uint64_t readPickId(int x, int y) const;

    /**
     * @brief Read a region centered at (cx,cy) with given radius.
     *
     * Returns non-zero pickIds sorted by distance from center (center-first).
     * Default radius = 6 → 13×13 = 169 pixel region.
     */
    [[nodiscard]] std::vector<uint64_t> readPickRegion(int cx, int cy,
                                                       int radius = 6) const;

private:
    void createAttachments();

    GLuint m_fbo{0};
    GLuint m_colorTex{0};
    GLuint m_depthRbo{0};
    int m_width{0};
    int m_height{0};
};

} // namespace OpenGeoLab::Render
