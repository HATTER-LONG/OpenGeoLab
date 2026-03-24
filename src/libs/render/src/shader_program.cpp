/**
 * @file shader_program.cpp
 * @brief Implements the GLSL shader program wrapper.
 */
#include <opengeolab/render/shader_program.hpp>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <utility>

namespace OpenGeoLab::Render {

namespace {

void logError(const std::string& message) { std::cerr << message << '\n'; }

[[nodiscard]] int uniformLocation(uint32_t program_id, std::string_view name) {
    const std::string uniform_name{name};
    return glGetUniformLocation(program_id, uniform_name.c_str());
}

} // namespace

ShaderProgram::~ShaderProgram() {
    if(programId_ != 0U) {
        glDeleteProgram(programId_);
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : programId_(other.programId_) {
    other.programId_ = 0U;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if(this == &other) {
        return *this;
    }

    if(programId_ != 0U) {
        glDeleteProgram(programId_);
    }

    programId_ = other.programId_;
    other.programId_ = 0U;
    return *this;
}

bool ShaderProgram::compile(std::string_view vertexSrc, std::string_view fragmentSrc) {
    const uint32_t vertex_shader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    if(vertex_shader == 0U) {
        return false;
    }

    const uint32_t fragment_shader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if(fragment_shader == 0U) {
        glDeleteShader(vertex_shader);
        return false;
    }

    const uint32_t program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);

    int link_status = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, &link_status);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if(link_status == 0) {
        int info_log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

        std::string info_log(static_cast<std::size_t>(std::max(info_log_length, 1)), '\0');
        glGetProgramInfoLog(program_id, info_log_length, nullptr, info_log.data());
        logError("Failed to link shader program: " + info_log);
        glDeleteProgram(program_id);
        return false;
    }

    if(programId_ != 0U) {
        glDeleteProgram(programId_);
    }
    programId_ = program_id;
    return true;
}

void ShaderProgram::use() const { glUseProgram(programId_); }

uint32_t ShaderProgram::id() const { return programId_; }

void ShaderProgram::setMat4(std::string_view name, const glm::mat4& value) const {
    glUniformMatrix4fv(uniformLocation(programId_, name), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setVec3(std::string_view name, const glm::vec3& value) const {
    glUniform3fv(uniformLocation(programId_, name), 1, glm::value_ptr(value));
}

void ShaderProgram::setVec4(std::string_view name, const glm::vec4& value) const {
    glUniform4fv(uniformLocation(programId_, name), 1, glm::value_ptr(value));
}

void ShaderProgram::setFloat(std::string_view name, float value) const {
    glUniform1f(uniformLocation(programId_, name), value);
}

void ShaderProgram::setInt(std::string_view name, int value) const {
    glUniform1i(uniformLocation(programId_, name), value);
}

uint32_t ShaderProgram::compileShader(uint32_t type, std::string_view source) {
    const uint32_t shader_id = glCreateShader(type);
    const std::array<const char*, 1> source_ptrs{source.data()};
    const std::array<int, 1> source_lengths{static_cast<int>(source.size())};
    glShaderSource(shader_id, static_cast<int>(source_ptrs.size()), source_ptrs.data(),
                   source_lengths.data());
    glCompileShader(shader_id);

    int compile_status = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
    if(compile_status != 0) {
        return shader_id;
    }

    int info_log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    std::string info_log(static_cast<std::size_t>(std::max(info_log_length, 1)), '\0');
    glGetShaderInfoLog(shader_id, info_log_length, nullptr, info_log.data());
    logError("Failed to compile shader stage: " + info_log);
    glDeleteShader(shader_id);
    return 0;
}

} // namespace OpenGeoLab::Render
