/**
 * @file picking_pass.hpp
 * @brief Integer-encoded picking pass using RG32UI framebuffer
 *
 * Encodes RenderEntityType (low 8 bits) and uid56 (high 56 bits) into a
 * 64-bit packed value per fragment for precise entity identification under
 * cursor. The GPU outputs this as uvec2 (two uint32 values) via an RG32UI
 * color attachment. Entity ID is baked into each vertex's m_uid attribute.
 */

#pragma once

#include "render/render_pass.hpp"
#include "render/render_types.hpp"
#include "render/renderable.hpp"

#include <QOpenGLShaderProgram>
#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Encode/decode helpers for the 64-bit pick ID.
 *
 * Layout: bits [63..8] = uid (56 bits), bits [7..0] = type (8 bits)
 * Delegates to RenderUID for encoding/decoding.
 */
struct PickIdCodec {
    static constexpr uint64_t encode(RenderEntityType type, uint64_t uid56) {
        return RenderUID::encode(type, uid56).m_packed;
    }

    struct Decoded {
        RenderEntityType m_type{RenderEntityType::None};
        uint64_t m_uid56{0};
    };

    static constexpr Decoded decode(uint64_t packed) {
        const RenderUID uid{packed};
        return Decoded{uid.type(), uid.uid56()};
    }
};

/**
 * @brief Picking pass that renders entity IDs to an RG32UI FBO.
 *
 * With batched rendering, entity UIDs are baked into vertex attributes.
 * The pick shader reads aUid per-vertex and outputs it as uvec2 to the
 * RG32UI color attachment. Each category needs only one draw call instead
 * of per-entity uniforms.
 *
 * Provides readPixel() to retrieve the 64-bit pick ID at a given screen
 * position.
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
     * @return Raw uint64 pick value (0 = background)
     */
    [[nodiscard]] uint64_t readPixel(QOpenGLFunctions& gl, int x, int y) const;

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
        QOpenGLFunctions& gl, int x, int y, int w, int h, std::vector<uint64_t>& out_pixels) const;

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
    int m_pickPointSizeLoc{-1};

    // Shader for edge picking (geometry shader for thick lines)
    std::unique_ptr<QOpenGLShaderProgram> m_pickEdgeShader;
    int m_pickEdgeMvpLoc{-1};
    int m_pickEdgeViewportLoc{-1};
    int m_pickEdgeThicknessLoc{-1};

    // FBO with RG32UI color attachment + depth
    GLuint m_fbo{0};
    GLuint m_colorTex{0};
    GLuint m_depthRbo{0};
    QSize m_fboSize;

    mutable GLint m_prevFbo{0}; ///< Saved FBO binding for restore (mutable for const bind/unbind)
};

} // namespace OpenGeoLab::Render
