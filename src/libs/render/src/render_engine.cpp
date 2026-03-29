#include <opengeolab/render/render_engine.hpp>

#include <opengeolab/core/logger.hpp>

#include <glad/gl.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QFile>
#include <QOpenGLContext>

#include <cstddef>
#include <string>

namespace OpenGeoLab::Render {

namespace {

std::string loadShaderSource(const QString& resource_path) {
    QFile file(resource_path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Core::getLogger()->error("Failed to open shader resource: {}", resource_path.toStdString());
        return {};
    }
    return file.readAll().toStdString();
}

} // namespace

void RenderEngine::initialize() {
    if(m_initialized) {
        return;
    }

    // Load GL function pointers via Qt's context-aware loader
    int version = gladLoadGL([](const char* name) -> GLADapiproc {
        auto* ctx = QOpenGLContext::currentContext();
        if(ctx == nullptr) {
            return nullptr;
        }
        return reinterpret_cast<GLADapiproc>(ctx->getProcAddress(name));
    });
    if(version == 0) {
        Core::getLogger()->error("gladLoadGL failed — no usable OpenGL context");
        return;
    }
    Core::getLogger()->info("OpenGL {}.{} loaded via glad", GLAD_VERSION_MAJOR(version),
                            GLAD_VERSION_MINOR(version));

    // Compile shaders from Qt resources
    std::string phong_vs = loadShaderSource(QStringLiteral(":/shaders/phong.vert"));
    std::string phong_fs = loadShaderSource(QStringLiteral(":/shaders/phong.frag"));
    std::string edge_vs = loadShaderSource(QStringLiteral(":/shaders/edge.vert"));
    std::string edge_fs = loadShaderSource(QStringLiteral(":/shaders/edge.frag"));

    if(!m_phongShader.compile(phong_vs, phong_fs)) {
        Core::getLogger()->error("Failed to compile phong shader");
        return;
    }
    if(!m_edgeShader.compile(edge_vs, edge_fs)) {
        Core::getLogger()->error("Failed to compile edge shader");
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.15f, 0.15f, 0.18f, 1.f);

    m_initialized = true;
    Core::getLogger()->info("RenderEngine initialized");
}

void RenderEngine::resize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
}

void RenderEngine::render(const RenderScene& scene, const Camera& camera) {
    if(!m_initialized) {
        return;
    }

    // Reset clear color every frame (Qt scene graph may reset GL state)
    glClearColor(0.15f, 0.15f, 0.18f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Pass 1: Phong surfaces — push surfaces back to avoid edge z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.f, 1.f);
    renderSurfaces(scene, camera);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Pass 2: Edge wireframe at normal depth
    renderEdges(scene, camera);

    // Pass 3: Point vertices on top
    renderPoints(scene, camera);
}

bool RenderEngine::isInitialized() const { return m_initialized; }

// ── Private ─────────────────────────────────────────────────────────────────

void RenderEngine::renderSurfaces(const RenderScene& scene, const Camera& camera) {
    float aspect =
        (m_height > 0) ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.f;
    glm::mat4 view = camera.viewMatrix();
    glm::mat4 proj = camera.projectionMatrix(aspect);

    m_phongShader.bind();
    m_phongShader.setUniform("uView", view);
    m_phongShader.setUniform("uProjection", proj);
    m_phongShader.setUniform("uEyePos", camera.position());

    // Headlight: light direction follows the camera view direction
    glm::vec3 light_dir = glm::normalize(camera.position() - camera.target());
    m_phongShader.setUniform("uLightDir", light_dir);

    for(const auto& node : scene.nodes()) {
        if(!node.visible || node.surfaces.empty()) {
            continue;
        }

        glm::mat4 model = node.modelMatrix;
        glm::mat3 normal_mat = glm::inverseTranspose(glm::mat3(model));

        m_phongShader.setUniform("uModel", model);
        m_phongShader.setUniform("uNormalMatrix", normal_mat);

        for(std::size_t i = 0; i < node.surfaces.size(); ++i) {
            const auto& c = (i < node.surfaceColors.size())
                                ? node.surfaceColors[i]
                                : std::array<float, 4>{node.defaultColor[0], node.defaultColor[1],
                                                       node.defaultColor[2], node.defaultColor[3]};
            m_phongShader.setUniform("uObjectColor", glm::vec4(c[0], c[1], c[2], c[3]));
            node.surfaces[i].draw();
        }
    }

    m_phongShader.release();
}

void RenderEngine::renderEdges(const RenderScene& scene, const Camera& camera) {
    float aspect =
        (m_height > 0) ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.f;
    glm::mat4 view = camera.viewMatrix();
    glm::mat4 proj = camera.projectionMatrix(aspect);

    m_edgeShader.bind();
    m_edgeShader.setUniform("uView", view);
    m_edgeShader.setUniform("uProjection", proj);
    glLineWidth(1.5f);

    for(const auto& node : scene.nodes()) {
        if(!node.visible || node.edges.empty()) {
            continue;
        }

        // Only draw edges if style includes wireframe
        if(node.style != Core::RenderStyle::Wireframe &&
           node.style != Core::RenderStyle::SolidWithEdges) {
            continue;
        }

        m_edgeShader.setUniform("uModel", node.modelMatrix);
        m_edgeShader.setUniform("uLineColor", glm::vec4(node.edgeColor[0], node.edgeColor[1],
                                                        node.edgeColor[2], node.edgeColor[3]));

        for(const auto& edge : node.edges) {
            edge.drawLines();
        }
    }

    m_edgeShader.release();
}

void RenderEngine::renderPoints(const RenderScene& scene, const Camera& camera) {
    float aspect =
        (m_height > 0) ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.f;
    glm::mat4 view = camera.viewMatrix();
    glm::mat4 proj = camera.projectionMatrix(aspect);

    m_edgeShader.bind();
    m_edgeShader.setUniform("uView", view);
    m_edgeShader.setUniform("uProjection", proj);

    glEnable(GL_PROGRAM_POINT_SIZE);

    for(const auto& node : scene.nodes()) {
        if(!node.visible || node.points.empty()) {
            continue;
        }

        m_edgeShader.setUniform("uModel", node.modelMatrix);
        m_edgeShader.setUniform("uLineColor",
                                glm::vec4(node.vertexColor[0], node.vertexColor[1],
                                           node.vertexColor[2], node.vertexColor[3]));
        m_edgeShader.setUniform("uPointSize", node.pointSize);

        for(const auto& pt : node.points) {
            pt.drawPoints();
        }
    }

    glDisable(GL_PROGRAM_POINT_SIZE);
    m_edgeShader.release();
}

} // namespace OpenGeoLab::Render
