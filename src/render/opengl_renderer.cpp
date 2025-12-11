/**
 * @file opengl_renderer.cpp
 * @brief Implementation of OpenGL renderer for 3D geometry
 */

#include <core/logger.hpp>
#include <render/opengl_renderer.hpp>

#include <QMatrix4x4>

namespace OpenGeoLab {
namespace Rendering {

// ============================================================================
// Shader Sources
// ============================================================================

const char* OpenGLRenderer::vertexShaderSource() {
    return R"(
        #version 120
        
        // Vertex attributes
        attribute vec3 aPos;
        attribute vec3 aNormal;
        attribute vec3 aColor;
        
        // Transformation matrices
        uniform mat4 uMVP;
        uniform mat4 uModel;
        uniform mat3 uNormalMatrix;
        
        // Color override (alpha = 0 means use vertex colors)
        uniform vec4 uColorOverride;
        
        // Outputs to fragment shader
        varying vec3 vWorldPos;
        varying vec3 vNormal;
        varying vec3 vColor;
        
        void main() {
            // Transform position to clip space
            gl_Position = uMVP * vec4(aPos, 1.0);
            
            // Transform position to world space for lighting
            vWorldPos = vec3(uModel * vec4(aPos, 1.0));
            
            // Transform normal to world space
            vNormal = normalize(uNormalMatrix * aNormal);
            
            // Use color override or vertex color
            if (uColorOverride.a > 0.0) {
                vColor = uColorOverride.rgb;
            } else {
                vColor = aColor;
            }
        }
    )";
}

const char* OpenGLRenderer::fragmentShaderSource() {
    return R"(
        #version 120
        
        // Maximum number of lights
        #define MAX_LIGHTS 4
        
        // Light types
        #define LIGHT_DIRECTIONAL 0
        #define LIGHT_POINT 1
        #define LIGHT_HEADLIGHT 2
        
        // Inputs from vertex shader
        varying vec3 vWorldPos;
        varying vec3 vNormal;
        varying vec3 vColor;
        
        // Camera position for specular calculation
        uniform vec3 uCameraPos;
        
        // Material properties
        uniform vec3 uMaterialAmbient;
        uniform vec3 uMaterialDiffuse;
        uniform vec3 uMaterialSpecular;
        uniform float uMaterialShininess;
        uniform bool uUseVertexColors;
        
        // Ambient light
        uniform vec3 uAmbientColor;
        uniform float uAmbientIntensity;
        
        // Light arrays
        uniform int uLightCount;
        uniform int uLightTypes[MAX_LIGHTS];
        uniform vec3 uLightPositions[MAX_LIGHTS];
        uniform vec3 uLightColors[MAX_LIGHTS];
        uniform float uLightIntensities[MAX_LIGHTS];
        
        vec3 calculateLight(int lightIndex, vec3 normal, vec3 viewDir, vec3 baseColor) {
            vec3 lightDir;
            float attenuation = 1.0;
            
            int lightType = uLightTypes[lightIndex];
            
            if (lightType == LIGHT_DIRECTIONAL) {
                // Directional light - position is direction
                lightDir = normalize(uLightPositions[lightIndex]);
            } else if (lightType == LIGHT_POINT) {
                // Point light
                vec3 toLight = uLightPositions[lightIndex] - vWorldPos;
                float distance = length(toLight);
                lightDir = normalize(toLight);
                attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
            } else if (lightType == LIGHT_HEADLIGHT) {
                // Headlight - from camera position
                lightDir = normalize(uCameraPos - vWorldPos);
            } else {
                return vec3(0.0);
            }
            
            vec3 lightColor = uLightColors[lightIndex];
            float intensity = uLightIntensities[lightIndex];
            
            // Diffuse component
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * lightColor * baseColor * intensity;
            
            // Specular component (Blinn-Phong)
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0), uMaterialShininess);
            vec3 specular = spec * lightColor * uMaterialSpecular * intensity;
            
            return (diffuse + specular) * attenuation;
        }
        
        void main() {
            // Normalize interpolated normal
            vec3 normal = normalize(vNormal);
            
            // View direction
            vec3 viewDir = normalize(uCameraPos - vWorldPos);
            
            // Base color (from vertex or material)
            vec3 baseColor = uUseVertexColors ? vColor : uMaterialDiffuse;
            
            // Start with ambient
            vec3 ambient = uAmbientColor * uAmbientIntensity * baseColor;
            vec3 result = ambient;
            
            // Add contribution from each light
            for (int i = 0; i < MAX_LIGHTS; i++) {
                if (i >= uLightCount) break;
                result += calculateLight(i, normal, viewDir, baseColor);
            }
            
            // Output final color
            gl_FragColor = vec4(result, 1.0);
        }
    )";
}

// ============================================================================
// OpenGLRenderer Implementation
// ============================================================================

OpenGLRenderer::OpenGLRenderer() : m_camera(std::make_unique<Camera>()) {
    // Setup default CAD lighting
    m_lighting.setupCADLighting();
}

OpenGLRenderer::~OpenGLRenderer() { delete m_program; }

void OpenGLRenderer::setGeometryData(std::shared_ptr<Geometry::GeometryData> geometry_data) {
    m_geometryData = geometry_data;
    m_needsBufferUpdate = true;

    LOG_INFO("Geometry data set: {} vertices, {} indices",
             geometry_data ? geometry_data->vertexCount() : 0,
             geometry_data ? geometry_data->indexCount() : 0);
}

void OpenGLRenderer::setColorOverride(const QColor& color) {
    if(m_colorOverride != color) {
        m_colorOverride = color;
        LOG_DEBUG("Color override set to: ({}, {}, {}, {})", color.red(), color.green(),
                  color.blue(), color.alpha());
    }
}

void OpenGLRenderer::setMaterial(const Material& material) { m_material = material; }

void OpenGLRenderer::rotateModel(float delta_yaw, float delta_pitch) {
    // Use arcball-style rotation that doesn't have gimbal lock issues
    // The rotation is accumulated in a way that feels natural regardless of current orientation

    // Create incremental rotation quaternions
    QQuaternion yaw_rotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), delta_yaw);
    QQuaternion pitch_rotation = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), delta_pitch);

    // Apply rotations: pitch first (local), then yaw (world)
    // This gives intuitive turntable behavior
    m_modelRotation = yaw_rotation * m_modelRotation * pitch_rotation;
    m_modelRotation.normalize();
}

void OpenGLRenderer::setModelRotation(float yaw, float pitch) {
    // Convert Euler angles to quaternion
    QQuaternion yaw_q = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), yaw);
    QQuaternion pitch_q = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), pitch);
    m_modelRotation = yaw_q * pitch_q;
    m_modelRotation.normalize();
}

void OpenGLRenderer::modelRotation(float& yaw, float& pitch) const {
    // Extract approximate Euler angles from quaternion
    // Note: This is approximate and may have gimbal lock issues at extreme angles
    QVector3D euler = m_modelRotation.toEulerAngles();
    yaw = euler.y();
    pitch = euler.x();
}

void OpenGLRenderer::resetModelRotation() { m_modelRotation = QQuaternion(); }

void OpenGLRenderer::setModelCenter(const QVector3D& center) { m_modelCenter = center; }

QMatrix4x4 OpenGLRenderer::modelMatrix() const {
    QMatrix4x4 model;
    model.setToIdentity();

    // Rotate around model center using quaternion:
    // 1. Translate model center to origin
    // 2. Apply rotation (from quaternion)
    // 3. Translate back
    model.translate(m_modelCenter);
    model.rotate(m_modelRotation);
    model.translate(-m_modelCenter);

    return model;
}

void OpenGLRenderer::init() {
    LOG_TRACE("OpenGLRenderer::init() called");

    if(!m_program && m_window) {
        LOG_INFO("Initializing OpenGL renderer resources");

        QSGRendererInterface* rif = m_window->rendererInterface();
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::OpenGL);

        initializeOpenGLFunctions();

        // Create shader program
        createShaderProgram();
        if(!m_program) {
            LOG_ERROR("Failed to create shader program");
            return;
        }

        // Create buffers if geometry data is available
        if(m_geometryData && m_needsBufferUpdate) {
            createBuffers();
            m_needsBufferUpdate = false;
        }

        m_initialized = true;
        LOG_INFO("OpenGL renderer initialization complete");
    }
}

void OpenGLRenderer::createShaderProgram() {
    m_program = new QOpenGLShaderProgram();

    bool vertex_ok =
        m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource());
    if(!vertex_ok) {
        LOG_ERROR("Failed to compile vertex shader: {}", m_program->log().toStdString());
        delete m_program;
        m_program = nullptr;
        return;
    }

    bool fragment_ok = m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,
                                                                   fragmentShaderSource());
    if(!fragment_ok) {
        LOG_ERROR("Failed to compile fragment shader: {}", m_program->log().toStdString());
        delete m_program;
        m_program = nullptr;
        return;
    }

    // Bind attribute locations
    m_program->bindAttributeLocation("aPos", 0);
    m_program->bindAttributeLocation("aNormal", 1);
    m_program->bindAttributeLocation("aColor", 2);

    bool link_ok = m_program->link();
    if(!link_ok) {
        LOG_ERROR("Failed to link shader program: {}", m_program->log().toStdString());
        delete m_program;
        m_program = nullptr;
        return;
    }

    LOG_INFO("Shader program created and linked successfully");
}

void OpenGLRenderer::createBuffers() {
    if(!m_geometryData) {
        LOG_WARN("No geometry data available for buffer creation");
        return;
    }

    LOG_DEBUG("Creating VBO and EBO for geometry with {} vertices, {} indices",
              m_geometryData->vertexCount(), m_geometryData->indexCount());

    // Destroy old buffers if they exist
    if(m_vbo.isCreated()) {
        m_vbo.destroy();
    }
    if(m_ebo.isCreated()) {
        m_ebo.destroy();
    }

    // Create and populate vertex buffer
    m_vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(m_geometryData->vertices(), m_geometryData->vertexCount() * 9 * sizeof(float));
    m_vbo.release();

    LOG_DEBUG("VBO created with {} bytes", m_geometryData->vertexCount() * 9 * sizeof(float));

    // Create and populate index buffer if available
    if(m_geometryData->indices() && m_geometryData->indexCount() > 0) {
        m_ebo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_ebo.create();
        m_ebo.bind();
        m_ebo.allocate(m_geometryData->indices(),
                       m_geometryData->indexCount() * sizeof(unsigned int));
        m_ebo.release();

        LOG_DEBUG("EBO created with {} indices", m_geometryData->indexCount());
    }
}

void OpenGLRenderer::setupVertexAttributes() {
    int stride = 9 * sizeof(float); // Each vertex: 3(pos) + 3(normal) + 3(color)

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);

    // Normal attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(3 * sizeof(float)));

    // Color attribute (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(6 * sizeof(float)));
}

void OpenGLRenderer::uploadLightingUniforms() {
    // Material properties
    m_program->setUniformValue("uMaterialAmbient", m_material.ambient);
    m_program->setUniformValue("uMaterialDiffuse", m_material.diffuse);
    m_program->setUniformValue("uMaterialSpecular", m_material.specular);
    m_program->setUniformValue("uMaterialShininess", m_material.shininess);
    m_program->setUniformValue("uUseVertexColors", m_material.useVertexColors);

    // Ambient light
    m_program->setUniformValue("uAmbientColor", m_lighting.ambientColor());
    m_program->setUniformValue("uAmbientIntensity", m_lighting.ambientIntensity());

    // Upload light data
    int light_count = qMin(m_lighting.lightCount(), MAX_LIGHTS);
    m_program->setUniformValue("uLightCount", light_count);

    for(int i = 0; i < light_count; ++i) {
        const Light& light = m_lighting.lights()[i];

        QString type_name = QString("uLightTypes[%1]").arg(i);
        QString pos_name = QString("uLightPositions[%1]").arg(i);
        QString color_name = QString("uLightColors[%1]").arg(i);
        QString intensity_name = QString("uLightIntensities[%1]").arg(i);

        m_program->setUniformValue(type_name.toUtf8().constData(), static_cast<int>(light.type));
        m_program->setUniformValue(pos_name.toUtf8().constData(), light.position);
        m_program->setUniformValue(color_name.toUtf8().constData(), light.color);
        m_program->setUniformValue(intensity_name.toUtf8().constData(), light.intensity);
    }
}

void OpenGLRenderer::paint() {
    // Always clear background, even without geometry
    m_window->beginExternalCommands();

    // Setup viewport - use full viewport without offset issues
    int window_height = m_window->height() * m_window->devicePixelRatio();
    int viewport_y = window_height - m_viewportOffset.y() - m_viewportSize.height();

    glViewport(m_viewportOffset.x(), viewport_y, m_viewportSize.width(), m_viewportSize.height());

    // Clear with background color
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(),
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Skip rendering if no geometry
    if(!m_program || !m_geometryData) {
        LOG_TRACE("Skipping geometry render: program={}, geometryData={}",
                  static_cast<void*>(m_program), static_cast<void*>(m_geometryData.get()));
        m_window->endExternalCommands();
        return;
    }

    LOG_TRACE("OpenGLRenderer::paint() called");

    // Check if buffers need to be updated
    if(m_needsBufferUpdate) {
        createBuffers();
        m_needsBufferUpdate = false;
    }

    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    // Disable backface culling to show both sides of surfaces
    // This is important for CAD models where users need to see backfaces
    glDisable(GL_CULL_FACE);

    // Bind shader program
    m_program->bind();

    // Bind buffers
    m_vbo.bind();
    if(m_geometryData->indices()) {
        m_ebo.bind();
    }

    // Setup vertex attributes
    setupVertexAttributes();

    // Calculate aspect ratio
    float aspect =
        static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height());

    // Get matrices from camera
    QMatrix4x4 view = m_camera->viewMatrix();
    QMatrix4x4 projection = m_camera->projectionMatrix(aspect);

    // Get model matrix with rotation
    // Using model rotation instead of camera rotation provides:
    // 1. Consistent lighting (lights stay fixed in world space)
    // 2. No gimbal lock issues
    // 3. More intuitive rotation behavior
    QMatrix4x4 model = modelMatrix();

    QMatrix4x4 mvp = projection * view * model;

    // Calculate normal matrix (inverse transpose of model matrix upper 3x3)
    QMatrix3x3 normal_matrix = model.normalMatrix();

    // Upload transformation uniforms
    m_program->setUniformValue("uMVP", mvp);
    m_program->setUniformValue("uModel", model);
    m_program->setUniformValue("uNormalMatrix", normal_matrix);

    // Upload camera position for specular calculation
    m_program->setUniformValue("uCameraPos", m_camera->position());

    // Upload color override
    QVector4D color_override(m_colorOverride.redF(), m_colorOverride.greenF(),
                             m_colorOverride.blueF(), m_colorOverride.alphaF());
    m_program->setUniformValue("uColorOverride", color_override);

    // Upload lighting uniforms
    uploadLightingUniforms();

    // Draw geometry
    if(m_geometryData->indices() && m_geometryData->indexCount() > 0) {
        glDrawElements(GL_TRIANGLES, m_geometryData->indexCount(), GL_UNSIGNED_INT, nullptr);
        LOG_TRACE("Drew geometry with {} indices", m_geometryData->indexCount());
    } else {
        glDrawArrays(GL_TRIANGLES, 0, m_geometryData->vertexCount());
        LOG_TRACE("Drew geometry with {} vertices", m_geometryData->vertexCount());
    }

    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    m_program->release();

    m_window->endExternalCommands();
}

} // namespace Rendering
} // namespace OpenGeoLab
