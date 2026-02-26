/**
 * @file shader_program.hpp
 * @brief Simple OpenGL shader program wrapper
 */

#pragma once

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QVector3D>

namespace OpenGeoLab::Render {

/**
 * @brief Lightweight wrapper around an OpenGL shader program.
 *
 * Compiles vertex/fragment source, exposes bind/release and
 * common uniform setters.  All GL calls go through the current
 * context's QOpenGLFunctions.
 */
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    /**
     * @brief Compile and link a vertex/fragment shader pair.
     * @return true on success, false on compilation or link error
     */
    bool compile(const char* vertex_src, const char* fragment_src);

    /** @brief Activate this program for subsequent draw calls */
    void bind();

    /** @brief Deactivate this program */
    void release();

    // ── Uniform setters ──────────────────────────────────────────────────

    void setUniformMatrix4(const char* name, const QMatrix4x4& mat);
    void setUniformVec3(const char* name, const QVector3D& vec);
    void setUniformFloat(const char* name, float val);

    // ── Accessors ────────────────────────────────────────────────────────

    [[nodiscard]] GLuint programId() const { return m_program; }

private:
    GLuint m_program{0};
    bool m_compiled{false};
};

} // namespace OpenGeoLab::Render
