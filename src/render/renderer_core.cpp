/**
 * @file renderer_core.cpp
 * @brief RendererCore implementation - central rendering engine
 */

#include "render/renderer_core.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLShader>

namespace OpenGeoLab::Render {

// =============================================================================
// Shader sources (same as original SceneRenderer for compatibility)
// =============================================================================
namespace {

const char* const MESH_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in uint aUid;
layout(location = 4) in uint aUidHigh;

uniform mat4 uMVPMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;
uniform float uPointSize;

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vColor;

void main() {
    vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    vColor = aColor;
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
}
)";

const char* const MESH_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vColor;

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform int uUseLighting;
uniform int uUseOverrideColor;
uniform vec4 uOverrideColor;

out vec4 fragColor;

void main() {
    vec4 baseColor = (uUseOverrideColor != 0) ? uOverrideColor : vColor;

    if(uUseLighting == 0) {
        fragColor = baseColor;
        return;
    }

    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);

    vec3 norm = vNormal;
    float nlen = length(norm);
    if(nlen < 1e-6) {
        norm = vec3(0.0, 0.0, 1.0);
    } else {
        norm = norm / nlen;
    }
    vec3 lightDir = normalize(uLightPos - vWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    float specularStrength = 0.3;
    vec3 viewDir = normalize(uViewPos - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = (ambient + diffuse + specular) * baseColor.rgb;
    fragColor = vec4(result, baseColor.a);
}
)";

const char* const PICK_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in uint aUidLow;
layout(location = 4) in uint aUidHigh;

uniform mat4 uMVPMatrix;
uniform float uPointSize;

flat out uint vUidLow;
flat out uint vUidHigh;

void main() {
    gl_Position = uMVPMatrix * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
    vUidLow = aUidLow;
    vUidHigh = aUidHigh;
}
)";

const char* const PICK_EDGE_GEOMETRY_SHADER = R"(
#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

flat in uint vUidLow[];
flat in uint vUidHigh[];

uniform vec2 uViewport;
uniform float uThickness;

flat out uint gUidLow;
flat out uint gUidHigh;

void main() {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;

    if(p0.w == 0.0 || p1.w == 0.0) {
        return;
    }

    vec2 n0 = p0.xy / p0.w;
    vec2 n1 = p1.xy / p1.w;
    vec2 d = (n1 - n0) * uViewport;
    float len = length(d);
    if(len < 1e-6) {
        return;
    }

    vec2 dir = d / len;
    vec2 n = vec2(-dir.y, dir.x);
    vec2 offset_ndc = (n * (uThickness * 0.5)) / uViewport;

    vec2 off0 = offset_ndc * p0.w;
    vec2 off1 = offset_ndc * p1.w;

    gUidLow = vUidLow[0];
    gUidHigh = vUidHigh[0];

    gl_Position = p0 + vec4(off0, 0.0, 0.0);
    EmitVertex();
    gl_Position = p0 - vec4(off0, 0.0, 0.0);
    EmitVertex();
    gl_Position = p1 + vec4(off1, 0.0, 0.0);
    EmitVertex();
    gl_Position = p1 - vec4(off1, 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
}
)";

const char* const PICK_FRAGMENT_SHADER = R"(
#version 330 core
flat in uint vUidLow;
flat in uint vUidHigh;
layout(location = 0) out uvec2 outPickId;
void main() {
    outPickId = uvec2(vUidLow, vUidHigh);
}
)";

// Edge pick fragment (receives gUidLow/gUidHigh from geometry shader)
const char* const PICK_EDGE_FRAGMENT_SHADER = R"(
#version 330 core
flat in uint gUidLow;
flat in uint gUidHigh;
layout(location = 0) out uvec2 outPickId;
void main() {
    outPickId = uvec2(gUidLow, gUidHigh);
}
)";

// RG32UI picking fragment shader (integer-encoded picking, reads uid from vertex)
const char* const PICK_R32UI_FRAGMENT_SHADER = R"(
#version 430 core
flat in uint vUidLow;
flat in uint vUidHigh;
layout(location = 0) out uvec2 outPickId;
void main() {
    outPickId = uvec2(vUidLow, vUidHigh);
}
)";

// Outline/solid-color shader for stencil highlight (scales from entity centroid)
const char* const OUTLINE_VERTEX_SHADER = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uMVPMatrix;
uniform float uScale;
uniform vec3 uCenter;

void main() {
    vec3 scaledPos = uCenter + (aPosition - uCenter) * uScale;
    gl_Position = uMVPMatrix * vec4(scaledPos, 1.0);
}
)";

const char* const OUTLINE_FRAGMENT_SHADER = R"(
#version 330 core
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)";

std::unique_ptr<QOpenGLShaderProgram>
compileShader(const char* name, const char* vert, const char* frag, const char* geom = nullptr) {
    auto program = std::make_unique<QOpenGLShaderProgram>();
    program->addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
    if(geom) {
        program->addShaderFromSourceCode(QOpenGLShader::Geometry, geom);
    }
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
    if(!program->link()) {
        LOG_ERROR("RendererCore: Failed to link shader '{}': {}", name,
                  program->log().toStdString());
    }
    return program;
}

} // namespace

// =============================================================================
// RendererCore
// =============================================================================

RendererCore::RendererCore() { LOG_TRACE("RendererCore created"); }

RendererCore::~RendererCore() {
    cleanup();
    LOG_TRACE("RendererCore destroyed");
}

void RendererCore::initialize() {
    if(m_initialized) {
        return;
    }

    initializeOpenGLFunctions();
    setupDefaultShaders();

    // Initialize all registered passes
    for(auto& pass : m_passes) {
        pass->initialize(*this);
    }

    m_initialized = true;
    LOG_DEBUG("RendererCore: Initialized with {} passes", m_passes.size());
}

bool RendererCore::checkGLCapabilities() const {
    auto* ctx = QOpenGLContext::currentContext();
    if(!ctx) {
        LOG_ERROR("RendererCore: No current GL context for capability check");
        return false;
    }

    const auto format = ctx->format();
    const int major = format.majorVersion();
    const int minor = format.minorVersion();

    const int required_major =
#ifdef OGL_REQUIRED_MINIMUM_MAJOR
        OGL_REQUIRED_MINIMUM_MAJOR;
#else
        4;
#endif
    const int required_minor =
#ifdef OGL_REQUIRED_MINIMUM_MINOR
        OGL_REQUIRED_MINIMUM_MINOR;
#else
        3;
#endif

    if(major < required_major || (major == required_major && minor < required_minor)) {
        LOG_WARN("RendererCore: GL {}.{} found, require >= {}.{}; R32UI picking unavailable", major,
                 minor, required_major, required_minor);
        return false;
    }

    LOG_DEBUG("RendererCore: GL {}.{} meets requirements", major, minor);
    return true;
}

void RendererCore::cleanup() {
    for(auto& pass : m_passes) {
        pass->cleanup(*this);
    }

    m_batch.clear();
    m_shaders.clear();
    m_initialized = false;

    LOG_DEBUG("RendererCore: Cleanup complete");
}

void RendererCore::setViewportSize(const QSize& size) {
    if(m_viewportSize == size) {
        return;
    }
    m_viewportSize = size;

    if(m_initialized) {
        for(auto& pass : m_passes) {
            pass->resize(*this, size);
        }
    }
}

void RendererCore::uploadMeshData(const DocumentRenderData& data) { m_batch.upload(*this, data); }

void RendererCore::registerPass(std::unique_ptr<RenderPass> pass) {
    if(m_initialized) {
        pass->initialize(*this);
        pass->resize(*this, m_viewportSize);
    }
    LOG_DEBUG("RendererCore: Registered pass '{}'", pass->name());
    m_passes.push_back(std::move(pass));
}

RenderPass* RendererCore::findPass(const char* name) const {
    for(const auto& pass : m_passes) {
        if(std::string_view(pass->name()) == name) {
            return pass.get();
        }
    }
    return nullptr;
}

void RendererCore::render(const QVector3D& camera_pos,
                          const QMatrix4x4& view_matrix,
                          const QMatrix4x4& projection_matrix) {
    if(!m_initialized) {
        LOG_WARN("RendererCore: render() called before initialize()");
        return;
    }

    RenderPassContext ctx;
    ctx.m_core = this;
    ctx.m_viewportSize = m_viewportSize;
    ctx.m_aspectRatio = m_viewportSize.width() / static_cast<float>(m_viewportSize.height());
    ctx.m_matrices.m_view = view_matrix;
    ctx.m_matrices.m_projection = projection_matrix;
    ctx.m_matrices.m_mvp = projection_matrix * view_matrix; // identity model
    ctx.m_cameraPos = camera_pos;

    for(auto& pass : m_passes) {
        if(pass->isEnabled()) {
            pass->execute(*this, ctx);
        }
    }
}

QOpenGLShaderProgram* RendererCore::shader(const std::string& key) const {
    for(const auto& entry : m_shaders) {
        if(entry.m_key == key) {
            return entry.m_program.get();
        }
    }
    return nullptr;
}

void RendererCore::registerShader(const std::string& key,
                                  std::unique_ptr<QOpenGLShaderProgram> program) {
    // Replace if already exists
    for(auto& entry : m_shaders) {
        if(entry.m_key == key) {
            entry.m_program = std::move(program);
            return;
        }
    }
    m_shaders.push_back({key, std::move(program)});
}

void RendererCore::setupDefaultShaders() {
    registerShader("mesh", compileShader("mesh", MESH_VERTEX_SHADER, MESH_FRAGMENT_SHADER));
    registerShader("pick", compileShader("pick", PICK_VERTEX_SHADER, PICK_FRAGMENT_SHADER));
    registerShader("pick_edge",
                   compileShader("pick_edge", PICK_VERTEX_SHADER, PICK_EDGE_FRAGMENT_SHADER,
                                 PICK_EDGE_GEOMETRY_SHADER));
    registerShader("outline",
                   compileShader("outline", OUTLINE_VERTEX_SHADER, OUTLINE_FRAGMENT_SHADER));

    // Try R32UI pick shader only if GL >= 4.3
    if(checkGLCapabilities()) {
        registerShader("pick_r32ui",
                       compileShader("pick_r32ui", PICK_VERTEX_SHADER, PICK_R32UI_FRAGMENT_SHADER));
    }

    LOG_DEBUG("RendererCore: Default shaders compiled");
}

} // namespace OpenGeoLab::Render
