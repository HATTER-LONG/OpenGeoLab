/**
 * @file opengl_renderer.cpp
 * @brief Implementation of OpenGL renderer for 3D geometry
 */

#include "render/opengl_renderer.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

namespace OpenGeoLab {
namespace Render {

namespace {

const char* VERTEX_SHADER_SOURCE = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main() {
    vec4 worldPos = uModelMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    gl_Position = uMVP * vec4(aPosition, 1.0);
}
)";

const char* FRAGMENT_SHADER_SOURCE = R"(
#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uViewPos;
uniform vec3 uAmbientColor;
uniform float uAmbientIntensity;

uniform int uLightCount;
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];
uniform float uLightIntensities[4];
uniform int uLightTypes[4];

uniform vec3 uMaterialAmbient;
uniform vec3 uMaterialDiffuse;
uniform vec3 uMaterialSpecular;
uniform float uMaterialShininess;
uniform bool uUseVertexColors;

uniform vec4 uColorOverride;

out vec4 fragColor;

vec3 calculateLight(int index, vec3 normal, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir;
    float attenuation = 1.0;

    if (uLightTypes[index] == 0) {
        // Directional light
        lightDir = normalize(uLightPositions[index]);
    } else if (uLightTypes[index] == 1) {
        // Point light
        vec3 toLight = uLightPositions[index] - vWorldPos;
        float dist = length(toLight);
        lightDir = normalize(toLight);
        attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
    } else {
        // Headlight
        lightDir = viewDir;
    }

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColors[index] * uLightIntensities[index] * baseColor;

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), uMaterialShininess);
    vec3 specular = spec * uLightColors[index] * uLightIntensities[index] * uMaterialSpecular;

    return (diffuse + specular) * attenuation;
}

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vWorldPos);

    // Base color
    vec3 baseColor;
    if (uColorOverride.a > 0.0) {
        baseColor = uColorOverride.rgb;
    } else if (uUseVertexColors) {
        baseColor = vColor;
    } else {
        baseColor = uMaterialDiffuse;
    }

    // Ambient
    vec3 ambient = uAmbientColor * uAmbientIntensity * uMaterialAmbient * baseColor;

    // Accumulate light contributions
    vec3 lighting = ambient;
    for (int i = 0; i < uLightCount && i < 4; ++i) {
        lighting += calculateLight(i, normal, viewDir, baseColor);
    }

    // Gamma correction
    lighting = pow(lighting, vec3(1.0 / 2.2));

    fragColor = vec4(lighting, 1.0);
}
)";

} // anonymous namespace

OpenGLRenderer::OpenGLRenderer()
    : m_vbo(QOpenGLBuffer::VertexBuffer), m_ebo(QOpenGLBuffer::IndexBuffer) {
    m_camera = std::make_unique<Camera>();
    m_lighting.setupCADLighting();
}

OpenGLRenderer::~OpenGLRenderer() {
    if(m_program) {
        delete m_program;
        m_program = nullptr;
    }
    if(m_vbo.isCreated()) {
        m_vbo.destroy();
    }
    if(m_ebo.isCreated()) {
        m_ebo.destroy();
    }
}

void OpenGLRenderer::setRenderGeometry(std::shared_ptr<RenderGeometry> geometry) {
    m_renderGeometry = geometry;
    m_needsBufferUpdate = true;

    if(geometry && !geometry->isEmpty()) {
        m_modelCenter = geometry->center();
    }
}

void OpenGLRenderer::setViewportSize(const QSize& size) {
    m_viewportSize = size;
    m_camera->setViewportSize(size.width(), size.height());
    m_trackball.setViewportSize(size);
}

void OpenGLRenderer::setColorOverride(const QColor& color) { m_colorOverride = color; }

void OpenGLRenderer::setMaterial(const Material& material) { m_material = material; }

void OpenGLRenderer::rotateModelByQuaternion(const QQuaternion& rotation) {
    m_modelRotation = rotation * m_modelRotation;
}

void OpenGLRenderer::resetModelRotation() { m_modelRotation = QQuaternion(); }

QMatrix4x4 OpenGLRenderer::modelMatrix() const {
    QMatrix4x4 model;
    model.translate(m_modelCenter);
    model.rotate(m_modelRotation);
    model.translate(-m_modelCenter);
    return model;
}

void OpenGLRenderer::setModelCenter(const QVector3D& center) { m_modelCenter = center; }

void OpenGLRenderer::fitToView() {
    if(!m_renderGeometry || m_renderGeometry->isEmpty()) {
        return;
    }

    QVector3D minPt = m_renderGeometry->boundingBoxMin();
    QVector3D maxPt = m_renderGeometry->boundingBoxMax();
    m_camera->fitToBounds(minPt, maxPt, 1.5f);
    m_modelCenter = m_renderGeometry->center();
    resetModelRotation();
}

void OpenGLRenderer::init() {
    if(m_initialized) {
        return;
    }

    initializeOpenGLFunctions();
    createShaderProgram();

    m_vbo.create();
    m_ebo.create();

    m_initialized = true;
    LOG_INFO("OpenGLRenderer initialized");
}

void OpenGLRenderer::paint() {
    if(!m_initialized || !m_window) {
        return;
    }

    // Update buffers if needed
    if(m_needsBufferUpdate && m_renderGeometry) {
        createBuffers();
        m_needsBufferUpdate = false;
    }

    // Set viewport
    glViewport(m_viewportOffset.x(), m_viewportOffset.y(), m_viewportSize.width(),
               m_viewportSize.height());

    // Clear with background color
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(),
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Nothing to render if no geometry
    if(!m_renderGeometry || m_renderGeometry->isEmpty()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_program->bind();

    // Calculate matrices
    float aspectRatio =
        m_viewportSize.width() / static_cast<float>(std::max(1, m_viewportSize.height()));
    QMatrix4x4 model = modelMatrix();
    QMatrix4x4 view = m_camera->viewMatrix();
    QMatrix4x4 projection = m_camera->projectionMatrix(aspectRatio);
    QMatrix4x4 mvp = projection * view * model;
    QMatrix3x3 normalMatrix = model.normalMatrix();

    // Upload uniforms
    m_program->setUniformValue("uMVP", mvp);
    m_program->setUniformValue("uModelMatrix", model);
    m_program->setUniformValue("uNormalMatrix", normalMatrix);
    m_program->setUniformValue("uViewPos", m_camera->position());

    // Upload lighting uniforms
    uploadLightingUniforms();

    // Upload material uniforms
    m_program->setUniformValue("uMaterialAmbient", m_material.ambient);
    m_program->setUniformValue("uMaterialDiffuse", m_material.diffuse);
    m_program->setUniformValue("uMaterialSpecular", m_material.specular);
    m_program->setUniformValue("uMaterialShininess", m_material.shininess);
    m_program->setUniformValue("uUseVertexColors", m_material.useVertexColors);

    // Color override
    QVector4D colorOverride(m_colorOverride.redF(), m_colorOverride.greenF(),
                            m_colorOverride.blueF(), m_colorOverride.alphaF());
    m_program->setUniformValue("uColorOverride", colorOverride);

    // Bind buffers and draw
    m_vbo.bind();
    setupVertexAttributes();
    m_ebo.bind();

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_renderGeometry->indices.size()),
                   GL_UNSIGNED_INT, nullptr);

    m_ebo.release();
    m_vbo.release();
    m_program->release();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // Reset OpenGL state for Qt Quick (Qt 6 compatible way)
    // Unbind buffers and program to restore Qt's expected state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void OpenGLRenderer::createShaderProgram() {
    m_program = new QOpenGLShaderProgram();

    if(!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER_SOURCE)) {
        LOG_ERROR("Vertex shader compilation failed: {}", m_program->log().toStdString());
        return;
    }

    if(!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAGMENT_SHADER_SOURCE)) {
        LOG_ERROR("Fragment shader compilation failed: {}", m_program->log().toStdString());
        return;
    }

    if(!m_program->link()) {
        LOG_ERROR("Shader program linking failed: {}", m_program->log().toStdString());
        return;
    }

    LOG_INFO("Shader program created successfully");
}

void OpenGLRenderer::createBuffers() {
    if(!m_renderGeometry) {
        return;
    }

    // Upload vertex data
    m_vbo.bind();
    m_vbo.allocate(m_renderGeometry->vertices.data(),
                   static_cast<int>(m_renderGeometry->vertices.size() * sizeof(RenderVertex)));
    m_vbo.release();

    // Upload index data
    m_ebo.bind();
    m_ebo.allocate(m_renderGeometry->indices.data(),
                   static_cast<int>(m_renderGeometry->indices.size() * sizeof(uint32_t)));
    m_ebo.release();

    LOG_DEBUG("Buffers created: {} vertices, {} indices", m_renderGeometry->vertices.size(),
              m_renderGeometry->indices.size());
}

void OpenGLRenderer::setupVertexAttributes() {
    // Position attribute
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, offsetof(RenderVertex, position), 3,
                                  sizeof(RenderVertex));

    // Normal attribute
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(1, GL_FLOAT, offsetof(RenderVertex, normal), 3,
                                  sizeof(RenderVertex));

    // Color attribute
    m_program->enableAttributeArray(2);
    m_program->setAttributeBuffer(2, GL_FLOAT, offsetof(RenderVertex, color), 3,
                                  sizeof(RenderVertex));
}

void OpenGLRenderer::uploadLightingUniforms() {
    m_program->setUniformValue("uAmbientColor", m_lighting.ambientColor());
    m_program->setUniformValue("uAmbientIntensity", m_lighting.ambientIntensity());

    int lightCount = std::min(m_lighting.lightCount(), MAX_LIGHTS);
    m_program->setUniformValue("uLightCount", lightCount);

    for(int i = 0; i < lightCount; ++i) {
        const Light& light = m_lighting.lights()[i];

        QString posUniform = QString("uLightPositions[%1]").arg(i);
        QString colorUniform = QString("uLightColors[%1]").arg(i);
        QString intensityUniform = QString("uLightIntensities[%1]").arg(i);
        QString typeUniform = QString("uLightTypes[%1]").arg(i);

        // For headlight, use camera position as light position
        QVector3D lightPos = light.position;
        if(light.type == LightType::Headlight) {
            lightPos = m_camera->position();
        }

        m_program->setUniformValue(posUniform.toLatin1().constData(), lightPos);
        m_program->setUniformValue(colorUniform.toLatin1().constData(), light.color);
        m_program->setUniformValue(intensityUniform.toLatin1().constData(), light.intensity);
        m_program->setUniformValue(typeUniform.toLatin1().constData(),
                                   static_cast<int>(light.type));
    }
}

const char* OpenGLRenderer::vertexShaderSource() { return VERTEX_SHADER_SOURCE; }

const char* OpenGLRenderer::fragmentShaderSource() { return FRAGMENT_SHADER_SOURCE; }

} // namespace Render
} // namespace OpenGeoLab
