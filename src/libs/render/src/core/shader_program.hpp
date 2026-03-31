/**
 * @file shader_program.hpp
 * @brief GLSL shader program compile, link, and uniform upload
 */

#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string_view>

namespace OpenGeoLab::Render {

/**
 * @brief Compiles a vertex + fragment shader pair and exposes uniform setters.
 *
 * Lifetime: call create() once, use() per frame, destroy() on cleanup.
 * All GL calls require a current context.
 */
class ShaderProgram final {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /**
     * @brief Compile vertex + fragment source and link into a program.
     * @return true on success, false on compile/link error (logged via Core::getLogger).
     */
    bool create(std::string_view vertex_src, std::string_view fragment_src);

    /** @brief Activate this program for subsequent draw calls. */
    void use() const;

    /** @brief Delete the GL program object. */
    void destroy();

    /** @brief Raw GL program id (0 if not created). */
    [[nodiscard]] GLuint id() const { return m_program; }

    // ── Uniform setters ──

    void setMat4(std::string_view name, const glm::mat4& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;
    void setFloat(std::string_view name, float value) const;
    void setInt(std::string_view name, int value) const;

private:
    /** @brief Compile a single shader stage. Returns 0 on error. */
    static GLuint compileShader(GLenum type, std::string_view source);

    GLuint m_program{0};
};

} // namespace OpenGeoLab::Render
