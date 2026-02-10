/**
 * @file picking_pass.hpp
 * @brief Integer-encoded picking pass using GL_R32UI framebuffer
 *
 * Encodes EntityType (low 8 bits) and EntityUID (high 24 bits) into a
 * single uint32 per fragment for precise entity identification under cursor.
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "render/render_pass.hpp"
#include "render/renderable.hpp"

#include <QOpenGLShaderProgram>
#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Encode/decode helpers for the 32-bit pick ID.
 *
 * Layout: bits [31..8] = uid (24 bits), bits [7..0] = type (8 bits)
 */
struct PickIdCodec {
    static constexpr uint32_t encode(Geometry::EntityType type, Geometry::EntityUID uid) {
        return ((static_cast<uint32_t>(uid) & 0xFFFFFFu) << 8) |
               (static_cast<uint32_t>(type) & 0xFFu);
    }

    struct Decoded {
        Geometry::EntityType m_type{Geometry::EntityType::None};
        Geometry::EntityUID m_uid{Geometry::INVALID_ENTITY_UID};
    };

    static constexpr Decoded decode(uint32_t packed) {
        Decoded d;
        d.m_type = static_cast<Geometry::EntityType>(packed & 0xFFu);
        d.m_uid = static_cast<Geometry::EntityUID>((packed >> 8) & 0xFFFFFFu);
        return d;
    }
};

/**
 * @brief Picking pass that renders entity IDs to a GL_R32UI FBO.
 *
 * Provides readPixel() to retrieve the pick ID at a given screen position.
 * Uses PBO for async readback when supported.
 */
class PickingPass : public RenderPass {
public:
    PickingPass() = default;
    ~PickingPass() override = default;

    [[nodiscard]] const char* name() const override { return "PickingPass"; }

    void initialize(QOpenGLFunctions& gl) override;
    void resize(QOpenGLFunctions& gl, const QSize& size) override;
    void execute(QOpenGLFunctions& gl, const RenderPassContext& ctx) override;
    void cleanup(QOpenGLFunctions& gl) override;

    /**
     * @brief Read a single pixel from the pick FBO.
     * @param gl OpenGL functions
     * @param x Pixel x coordinate (in FBO space)
     * @param y Pixel y coordinate (in FBO space, 0=bottom)
     * @return Raw uint32 pick value (0 = background)
     */
    [[nodiscard]] uint32_t readPixel(QOpenGLFunctions& gl, int x, int y) const;

    /**
     * @brief Read a rectangular region of pixels from the pick FBO.
     * @param gl OpenGL functions
     * @param x Region left (FBO space)
     * @param y Region bottom (FBO space)
     * @param w Region width
     * @param h Region height
     * @param out_pixels Output buffer (will be resized to w*h)
     */
    void readRegion(
        QOpenGLFunctions& gl, int x, int y, int w, int h, std::vector<uint32_t>& out_pixels) const;

    /**
     * @brief Bind the pick FBO so passes can render to it.
     */
    void bindFbo(QOpenGLFunctions& gl) const;

    /**
     * @brief Unbind the pick FBO.
     */
    void unbindFbo(QOpenGLFunctions& gl) const;

    [[nodiscard]] bool hasFbo() const { return m_fbo != 0; }

private:
    void createFbo(QOpenGLFunctions& gl, const QSize& size);
    void destroyFbo(QOpenGLFunctions& gl);

    // Shader for face/vertex picking (flat uint output)
    std::unique_ptr<QOpenGLShaderProgram> m_pickShader;
    int m_pickMvpLoc{-1};
    int m_pickIdLoc{-1};
    int m_pickPointSizeLoc{-1};

    // Shader for edge picking (geometry shader for thick lines)
    std::unique_ptr<QOpenGLShaderProgram> m_pickEdgeShader;
    int m_pickEdgeMvpLoc{-1};
    int m_pickEdgeIdLoc{-1};
    int m_pickEdgeViewportLoc{-1};
    int m_pickEdgeThicknessLoc{-1};

    // FBO with GL_R32UI color attachment + depth
    GLuint m_fbo{0};
    GLuint m_colorTex{0}; ///< GL_R32UI texture
    GLuint m_depthRbo{0};
    QSize m_fboSize;

    mutable GLint m_prevFbo{0}; ///< Saved FBO binding for restore (mutable for const bind/unbind)
};

} // namespace OpenGeoLab::Render
