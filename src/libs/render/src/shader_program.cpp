#include <opengeolab/render/shader_program.hpp>

#include <opengeolab/core/logger.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <utility>

namespace OpenGeoLab::Render {

ShaderProgram::~ShaderProgram() { destroy(); }

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : m_program(other.m_program) {
    other.m_program = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if(this != &other) {
        destroy();
        m_program = std::exchange(other.m_program, 0);
    }
    return *this;
}

bool ShaderProgram::compile(std::string_view vertex_source, std::string_view fragment_source) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertex_source);
    if(vs == 0) {
        return false;
    }

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragment_source);
    if(fs == 0) {
        glDeleteShader(vs);
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(success == 0) {
        GLint log_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<size_t>(log_len), '\0');
        glGetProgramInfoLog(program, log_len, nullptr, log.data());
        Core::getLogger()->error("Shader link error: {}", log);
        glDeleteProgram(program);
        return false;
    }

    destroy();
    m_program = program;
    return true;
}

void ShaderProgram::bind() const { glUseProgram(m_program); }

void ShaderProgram::release() const { glUseProgram(0); }

void ShaderProgram::setUniform(const char* name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setUniform(const char* name, const glm::mat3& mat) const {
    glUniformMatrix3fv(glGetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setUniform(const char* name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(vec));
}

void ShaderProgram::setUniform(const char* name, const glm::vec4& vec) const {
    glUniform4fv(glGetUniformLocation(m_program, name), 1, glm::value_ptr(vec));
}

void ShaderProgram::setUniform(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(m_program, name), value);
}

void ShaderProgram::setUniform(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(m_program, name), value);
}

GLuint ShaderProgram::programId() const { return m_program; }

// ── Private ─────────────────────────────────────────────────────────────────

GLuint ShaderProgram::compileShader(GLenum type, std::string_view source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.data();
    auto len = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(success == 0) {
        GLint log_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<size_t>(log_len), '\0');
        glGetShaderInfoLog(shader, log_len, nullptr, log.data());
        const char* type_str = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        Core::getLogger()->error("Shader compile error ({}): {}", type_str, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void ShaderProgram::destroy() {
    if(m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

} // namespace OpenGeoLab::Render
