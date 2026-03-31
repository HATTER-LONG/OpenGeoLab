#include "core/shader_program.hpp"

#include <opengeolab/core/logger.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <utility>

namespace OpenGeoLab::Render {

ShaderProgram::~ShaderProgram() { destroy(); }

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_program(std::exchange(other.m_program, 0)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        destroy();
        m_program = std::exchange(other.m_program, 0);
    }
    return *this;
}

GLuint ShaderProgram::compileShader(GLenum type, std::string_view source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.data();
    auto length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &src, &length);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::string::size_type>(logLen), '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        Core::getLogger()->error("Shader compile error: {}", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ShaderProgram::create(std::string_view vertexSrc, std::string_view fragmentSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    if (vs == 0) {
        return false;
    }

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (fs == 0) {
        glDeleteShader(vs);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::string::size_type>(logLen), '\0');
        glGetProgramInfoLog(m_program, logLen, nullptr, log.data());
        Core::getLogger()->error("Shader link error: {}", log);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }
    return true;
}

void ShaderProgram::use() const {
    glUseProgram(m_program);
}

void ShaderProgram::destroy() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void ShaderProgram::setMat4(std::string_view name, const glm::mat4& value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setVec3(std::string_view name, const glm::vec3& value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void ShaderProgram::setVec4(std::string_view name, const glm::vec4& value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform4fv(loc, 1, glm::value_ptr(value));
}

void ShaderProgram::setFloat(std::string_view name, float value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform1f(loc, value);
}

void ShaderProgram::setInt(std::string_view name, int value) const {
    GLint loc = glGetUniformLocation(m_program, std::string(name).c_str());
    glUniform1i(loc, value);
}

} // namespace OpenGeoLab::Render
