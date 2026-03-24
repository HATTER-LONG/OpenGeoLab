/**
 * @file shader_program.hpp
 * @brief Declares the RAII GLSL shader program wrapper.
 */
#pragma once

#include <opengeolab/render/render_export.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace OpenGeoLab::Render {

/**
 * @brief RAII wrapper for a GLSL shader program.
 *
 * Compiles vertex + fragment shaders from source strings and links them.
 * Provides convenience uniform setters.
 * Destructor releases GL resources.
 */
class OPENGEOLAB_RENDER_EXPORT ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /** @brief Compile vertex + fragment shaders and link. Returns true on success. */
    bool compile(std::string_view vertexSrc, std::string_view fragmentSrc);

    void use() const;
    [[nodiscard]] uint32_t id() const;

    void setMat4(std::string_view name, const glm::mat4& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;
    void setFloat(std::string_view name, float value) const;
    void setInt(std::string_view name, int value) const;

private:
    uint32_t programId_ = 0;

    /** @brief Compile a single shader stage. Returns shader ID or 0 on failure. */
    static uint32_t compileShader(uint32_t type, std::string_view source);
};

} // namespace OpenGeoLab::Render