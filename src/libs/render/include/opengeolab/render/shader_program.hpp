/**
 * @file shader_program.hpp
 * @brief RAII wrapper for OpenGL GLSL shader compilation and linking.
 */

#pragma once

#include <opengeolab/render/render_export.hpp>

#include <glad/gl.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

/// GLSL shader program RAII wrapper.
///
/// Non-copyable, move-only.  Destructor calls glDeleteProgram.
class OPENGEOLAB_RENDER_EXPORT ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    /// Compile and link from vertex + fragment source strings.
    /// @return true on success.
    bool compile(std::string_view vertex_source, std::string_view fragment_source);

    void bind() const;
    void release() const;

    // ── Uniform setters (program must be bound) ─────────────────────────
    void setUniform(const char* name, const glm::mat4& mat) const;
    void setUniform(const char* name, const glm::mat3& mat) const;
    void setUniform(const char* name, const glm::vec3& vec) const;
    void setUniform(const char* name, const glm::vec4& vec) const;
    void setUniform(const char* name, float value) const;
    void setUniform(const char* name, int value) const;

    [[nodiscard]] GLuint programId() const;

private:
    GLuint m_program{0};

    static GLuint compileShader(GLenum type, std::string_view source);
    void destroy();
};

} // namespace OpenGeoLab::Render
