#include "render_sceneImpl.hpp"
#include "render/render_scene_controller.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {
namespace {
struct VertexPC {
    float m_x;
    float m_y;
    float m_z;
    float m_r;
    float m_g;
    float m_b;
    float m_a;
};

bool isModeVisible(RenderDisplayMode mode, PrimitiveTopology topology) {
    switch(mode) {
    case RenderDisplayMode::Surface:
        return topology == PrimitiveTopology::Triangles;
    case RenderDisplayMode::Wireframe:
        return topology == PrimitiveTopology::Lines;
    case RenderDisplayMode::Points:
        return topology == PrimitiveTopology::Points;
    default:
        return true;
    }
}

GLenum toGlPrimitive(PrimitiveTopology topology) {
    switch(topology) {
    case PrimitiveTopology::Points:
        return GL_POINTS;
    case PrimitiveTopology::Lines:
        return GL_LINES;
    case PrimitiveTopology::Triangles:
        return GL_TRIANGLES;
    default:
        return GL_TRIANGLES;
    }
}
} // namespace

RenderSceneImpl::RenderSceneImpl() {
    LOG_DEBUG("RenderSceneImpl: Created new render scene instance");
}
RenderSceneImpl::~RenderSceneImpl() {
    LOG_DEBUG("RenderSceneImpl: Destroying render scene instance");
}

void RenderSceneImpl::initialize() { LOG_DEBUG("RenderSceneImpl: Initializing render scene"); }

[[nodiscard]] bool RenderSceneImpl::isInitialized() const { return m_initialized; }

void RenderSceneImpl::setViewportSize(const QSize& size) {
    m_viewportSize = size;
    LOG_DEBUG("RenderSceneImpl: Viewport size set to {}x{}", size.width(), size.height());
}

void RenderSceneImpl::processPicking(const PickingInput& input) {
    LOG_DEBUG("RenderSceneImpl: Processing picking at cursor position ({}, {}) with action {}",
              input.m_cursorPos.x(), input.m_cursorPos.y(), static_cast<int>(input.m_action));
}

void RenderSceneImpl::render(const QVector3D& camera_pos,
                             const QMatrix4x4& view_matrix,
                             const QMatrix4x4& projection_matrix) {
    auto* context = QOpenGLContext::currentContext();
    if(!context) {
        LOG_WARN("RenderSceneImpl: no active OpenGL context, skip rendering");
        return;
    }

    auto* gl = context->functions();
    if(!gl) {
        LOG_WARN("RenderSceneImpl: no OpenGL functions available");
        return;
    }

    if(!m_initialized) {
        gl->glEnable(GL_DEPTH_TEST);
        gl->glEnable(GL_BLEND);
        gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_initialized = true;
        LOG_DEBUG("RenderSceneImpl: OpenGL state initialized");
    }

    ensureGpuResources();
    if(!m_gpuReady) {
        LOG_WARN("RenderSceneImpl: GPU resources not ready");
        return;
    }

    if(m_viewportSize.width() > 0 && m_viewportSize.height() > 0) {
        gl->glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    }

    const auto& bucket = RenderSceneController::instance().renderData();
    const auto display_mode = RenderSceneController::instance().displayMode();
    const auto revision = RenderSceneController::instance().renderRevision();

    if(!m_hasUploadedData || revision != m_lastUploadedRevision ||
       display_mode != m_lastUploadedMode) {
        uploadBuckets(bucket, display_mode);
        m_lastUploadedRevision = revision;
        m_lastUploadedMode = display_mode;
        m_hasUploadedData = true;
    }

    QMatrix4x4 mvp = projection_matrix * view_matrix;

    gl->glClearColor(0.08F, 0.08F, 0.1F, 1.0F);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawBuckets(mvp);

    const auto count_visible = [display_mode](const RenderData& pass_data) {
        size_t visible = 0;
        for(const auto& primitive : pass_data.m_primitives) {
            if(primitive.m_visible && isModeVisible(display_mode, primitive.m_topology)) {
                ++visible;
            }
        }
        return visible;
    };

    const auto geometry_count = count_visible(bucket.m_geometryPass);
    const auto mesh_count = count_visible(bucket.m_meshPass);
    const auto post_count = count_visible(bucket.m_postPass);

    LOG_DEBUG("RenderSceneImpl: Rendering scene with camera at ({}, {}, {})", camera_pos.x(),
              camera_pos.y(), camera_pos.z());
    LOG_TRACE("RenderSceneImpl: mode={}, pass primitives geometry={}, mesh={}, post={}",
              static_cast<int>(display_mode), geometry_count, mesh_count, post_count);
}

void RenderSceneImpl::cleanup() {
    releaseGpuResources();
    m_initialized = false;
    m_hasUploadedData = false;
    m_lastUploadedRevision = 0;
    m_lastUploadedMode = RenderDisplayMode::Surface;
    LOG_DEBUG("RenderSceneImpl: Cleaning up render scene");
}

int RenderSceneImpl::topologyIndex(PrimitiveTopology topology) {
    switch(topology) {
    case PrimitiveTopology::Points:
        return 0;
    case PrimitiveTopology::Lines:
        return 1;
    case PrimitiveTopology::Triangles:
        return 2;
    default:
        return 2;
    }
}

void RenderSceneImpl::ensureGpuResources() {
    if(m_gpuReady) {
        return;
    }

    auto* context = QOpenGLContext::currentContext();
    if(!context) {
        return;
    }

    if(!m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                         R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec4 aColor;
        uniform mat4 uMvp;
        out vec4 vColor;
        void main() {
            gl_Position = uMvp * vec4(aPos, 1.0);
            vColor = aColor;
            gl_PointSize = 4.0;
        }
    )")) {
        LOG_ERROR("RenderSceneImpl: vertex shader compile failed: {}",
                  m_shader.log().toStdString());
        return;
    }

    if(!m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                         R"(
        #version 330 core
        in vec4 vColor;
        out vec4 fragColor;
        void main() {
            fragColor = vColor;
        }
    )")) {
        LOG_ERROR("RenderSceneImpl: fragment shader compile failed: {}",
                  m_shader.log().toStdString());
        return;
    }

    if(!m_shader.link()) {
        LOG_ERROR("RenderSceneImpl: shader link failed: {}", m_shader.log().toStdString());
        return;
    }

    auto* functions = context->functions();
    if(!functions) {
        LOG_ERROR("RenderSceneImpl: OpenGL functions unavailable during GPU init");
        return;
    }

    for(auto& gpu_bucket : m_topologyBuckets) {
        gpu_bucket.m_created =
            gpu_bucket.m_vao.create() && gpu_bucket.m_vbo.create() && gpu_bucket.m_ebo.create();
        if(!gpu_bucket.m_created) {
            LOG_ERROR("RenderSceneImpl: failed to create GPU bucket resources");
            return;
        }

        gpu_bucket.m_vao.bind();
        gpu_bucket.m_vbo.bind();
        gpu_bucket.m_ebo.bind();

        functions->glEnableVertexAttribArray(0);
        functions->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                                         static_cast<GLsizei>(sizeof(VertexPC)),
                                         reinterpret_cast<const void*>(offsetof(VertexPC, m_x)));
        functions->glEnableVertexAttribArray(1);
        functions->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                                         static_cast<GLsizei>(sizeof(VertexPC)),
                                         reinterpret_cast<const void*>(offsetof(VertexPC, m_r)));

        gpu_bucket.m_vao.release();
        gpu_bucket.m_vbo.release();
        gpu_bucket.m_ebo.release();
    }

    m_gpuReady = true;
    LOG_DEBUG("RenderSceneImpl: GPU bucket resources initialized (3 topology buckets)");
}

void RenderSceneImpl::uploadBuckets(const RenderBucket& bucket, RenderDisplayMode mode) {
    std::array<std::vector<VertexPC>, 3> vertices_by_topology;
    std::array<std::vector<uint32_t>, 3> indices_by_topology;

    const auto append_pass = [&](const RenderData& pass_data) {
        for(const auto& primitive : pass_data.m_primitives) {
            if(!primitive.m_visible || !isModeVisible(mode, primitive.m_topology)) {
                continue;
            }

            const int topo_idx = topologyIndex(primitive.m_topology);
            auto& vertices = vertices_by_topology[topo_idx];
            auto& indices = indices_by_topology[topo_idx];

            if((primitive.m_positions.size() % 3) != 0) {
                continue;
            }

            const uint32_t base_vertex = static_cast<uint32_t>(vertices.size());
            const uint32_t vertex_count = static_cast<uint32_t>(primitive.m_positions.size() / 3);
            vertices.reserve(vertices.size() + vertex_count);

            for(uint32_t i = 0; i < vertex_count; ++i) {
                const size_t offset = static_cast<size_t>(i) * 3;
                vertices.push_back(
                    VertexPC{primitive.m_positions[offset + 0], primitive.m_positions[offset + 1],
                             primitive.m_positions[offset + 2], primitive.m_color.m_r,
                             primitive.m_color.m_g, primitive.m_color.m_b, primitive.m_color.m_a});
            }

            if(!primitive.m_indices.empty()) {
                indices.reserve(indices.size() + primitive.m_indices.size());
                for(const auto index : primitive.m_indices) {
                    indices.push_back(base_vertex + index);
                }
            } else {
                indices.reserve(indices.size() + vertex_count);
                for(uint32_t i = 0; i < vertex_count; ++i) {
                    indices.push_back(base_vertex + i);
                }
            }
        }
    };

    append_pass(bucket.m_geometryPass);
    append_pass(bucket.m_meshPass);
    append_pass(bucket.m_postPass);

    for(int idx = 0; idx < 3; ++idx) {
        auto& gpu_bucket = m_topologyBuckets[idx];
        if(!gpu_bucket.m_created) {
            continue;
        }

        gpu_bucket.m_vao.bind();
        gpu_bucket.m_vbo.bind();
        gpu_bucket.m_ebo.bind();

        const auto& vertices = vertices_by_topology[idx];
        const auto& indices = indices_by_topology[idx];

        if(vertices.empty() || indices.empty()) {
            gpu_bucket.m_indexCount = 0;
        } else {
            gpu_bucket.m_vbo.allocate(vertices.data(),
                                      static_cast<int>(vertices.size() * sizeof(VertexPC)));
            gpu_bucket.m_ebo.allocate(indices.data(),
                                      static_cast<int>(indices.size() * sizeof(uint32_t)));
            gpu_bucket.m_indexCount = static_cast<int>(indices.size());
        }

        gpu_bucket.m_vao.release();
        gpu_bucket.m_vbo.release();
        gpu_bucket.m_ebo.release();
    }
}

void RenderSceneImpl::drawBuckets(const QMatrix4x4& mvp) {
    if(!m_gpuReady) {
        return;
    }

    if(!m_shader.bind()) {
        LOG_WARN("RenderSceneImpl: failed to bind shader program");
        return;
    }

    auto* context = QOpenGLContext::currentContext();
    auto* functions = context ? context->functions() : nullptr;
    if(!functions) {
        m_shader.release();
        return;
    }
    m_shader.setUniformValue("uMvp", mvp);

    functions->glLineWidth(1.5F);

    constexpr std::array<PrimitiveTopology, 3> topology_order = {
        PrimitiveTopology::Points,
        PrimitiveTopology::Lines,
        PrimitiveTopology::Triangles,
    };

    for(int idx = 0; idx < 3; ++idx) {
        auto& gpu_bucket = m_topologyBuckets[idx];
        if(!gpu_bucket.m_created || gpu_bucket.m_indexCount <= 0) {
            continue;
        }

        gpu_bucket.m_vao.bind();
        functions->glDrawElements(toGlPrimitive(topology_order[idx]), gpu_bucket.m_indexCount,
                                  GL_UNSIGNED_INT, nullptr);
        gpu_bucket.m_vao.release();
    }

    m_shader.release();
}

void RenderSceneImpl::releaseGpuResources() {
    for(auto& gpu_bucket : m_topologyBuckets) {
        if(gpu_bucket.m_created) {
            gpu_bucket.m_vao.destroy();
            gpu_bucket.m_vbo.destroy();
            gpu_bucket.m_ebo.destroy();
            gpu_bucket.m_indexCount = 0;
            gpu_bucket.m_created = false;
        }
    }

    if(m_shader.isLinked()) {
        m_shader.removeAllShaders();
    }

    m_gpuReady = false;
}

} // namespace OpenGeoLab::Render