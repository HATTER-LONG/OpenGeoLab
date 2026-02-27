/**
 * @file shader_program.cpp
 * @brief ShaderProgram implementation
 */

#include "shader_program.hpp"

#include "util/logger.hpp"

#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QVector3D>

namespace OpenGeoLab::Render {

// =============================================================================
// Lifecycle
// =============================================================================

ShaderProgram::~ShaderProgram() {
    if(m_program != 0) {
        auto* ctx = QOpenGLContext::currentContext();
        if(ctx) {
            ctx->functions()->glDeleteProgram(m_program);
            LOG_DEBUG("ShaderProgram: Deleted program {}", m_program);
        } else {
            LOG_ERROR("ShaderProgram: Cannot delete program {} — no current GL context", m_program);
        }
        m_program = 0;
        m_compiled = false;
    }
}

// =============================================================================
// Compilation
// =============================================================================

static bool compileShader(QOpenGLFunctions* f, GLenum type, const char* src, GLuint& out_shader) {
    out_shader = f->glCreateShader(type);
    f->glShaderSource(out_shader, 1, &src, nullptr);
    f->glCompileShader(out_shader);

    GLint success = 0;
    f->glGetShaderiv(out_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        GLint log_len = 0;
        f->glGetShaderiv(out_shader, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<std::size_t>(log_len), '\0');
        f->glGetShaderInfoLog(out_shader, log_len, nullptr, log.data());
        LOG_ERROR("ShaderProgram: Shader compilation failed (type={}):\n{}", type, log);
        f->glDeleteShader(out_shader);
        out_shader = 0;
        return false;
    }
    return true;
}

bool ShaderProgram::compile(const char* vertex_src, const char* fragment_src) {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    // Compile individual stages
    GLuint vs = 0;
    GLuint fs = 0;
    if(!compileShader(f, GL_VERTEX_SHADER, vertex_src, vs)) {
        return false;
    }
    if(!compileShader(f, GL_FRAGMENT_SHADER, fragment_src, fs)) {
        f->glDeleteShader(vs);
        return false;
    }

    // Link program
    m_program = f->glCreateProgram();
    f->glAttachShader(m_program, vs);
    f->glAttachShader(m_program, fs);
    f->glLinkProgram(m_program);

    // Shaders can be detached and deleted after linking
    f->glDetachShader(m_program, vs);
    f->glDetachShader(m_program, fs);
    f->glDeleteShader(vs);
    f->glDeleteShader(fs);

    GLint success = 0;
    f->glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if(!success) {
        GLint log_len = 0;
        f->glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<std::size_t>(log_len), '\0');
        f->glGetProgramInfoLog(m_program, log_len, nullptr, log.data());
        LOG_ERROR("ShaderProgram: Program link failed:\n{}", log);
        f->glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    m_compiled = true;
    LOG_DEBUG("ShaderProgram: Compiled and linked program {}", m_program);
    return true;
}

// =============================================================================
// Bind / Release
// =============================================================================

void ShaderProgram::bind() {
    if(m_compiled) {
        QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
        f->glUseProgram(m_program);
    }
}

void ShaderProgram::release() {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glUseProgram(0);
}

// =============================================================================
// Uniform setters
// =============================================================================

void ShaderProgram::setUniformMatrix4(const char* name, const QMatrix4x4& mat) {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    GLint loc = f->glGetUniformLocation(m_program, name);
    if(loc >= 0) {
        f->glUniformMatrix4fv(loc, 1, GL_FALSE, mat.constData());
    }
}

void ShaderProgram::setUniformVec3(const char* name, const QVector3D& vec) {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    GLint loc = f->glGetUniformLocation(m_program, name);
    if(loc >= 0) {
        f->glUniform3f(loc, vec.x(), vec.y(), vec.z());
    }
}

void ShaderProgram::setUniformVec4(const char* name, float r, float g, float b, float a) {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    GLint loc = f->glGetUniformLocation(m_program, name);
    if(loc >= 0) {
        f->glUniform4f(loc, r, g, b, a);
    }
}

void ShaderProgram::setUniformFloat(const char* name, float val) {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    GLint loc = f->glGetUniformLocation(m_program, name);
    if(loc >= 0) {
        f->glUniform1f(loc, val);
    }
}

void ShaderProgram::setUniformInt(const char* name, int val) {
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    GLint loc = f->glGetUniformLocation(m_program, name);
    if(loc >= 0) {
        f->glUniform1i(loc, val);
    }
}

void ShaderProgram::setUniformUvec2(const char* name, uint32_t x, uint32_t y) {
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    GLint loc = f->glGetUniformLocation(m_program, name);
    if(loc >= 0) {
        ef->glUniform2ui(loc, x, y);
    }
}

} // namespace OpenGeoLab::Render
