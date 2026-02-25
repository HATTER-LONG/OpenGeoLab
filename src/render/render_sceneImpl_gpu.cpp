/**
 * @file render_sceneImpl_gpu.cpp
 * @brief GPU/pass upload and draw implementation for RenderSceneImpl
 */

#include "render_sceneImpl.hpp"

#include "render_scene_impl_internal.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include <array>
#include <cstddef>

namespace OpenGeoLab::Render {

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
        layout(location = 2) in uvec2 aPick;
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
    auto* extra = context->extraFunctions();
    if(!functions) {
        LOG_ERROR("RenderSceneImpl: OpenGL functions unavailable during GPU init");
        return;
    }
    if(!extra) {
        LOG_ERROR("RenderSceneImpl: OpenGL extra functions unavailable during GPU init");
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
        functions->glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Detail::VertexPC)),
            reinterpret_cast<const void*>(offsetof(Detail::VertexPC, m_x)));
        functions->glEnableVertexAttribArray(1);
        functions->glVertexAttribPointer(
            1, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Detail::VertexPC)),
            reinterpret_cast<const void*>(offsetof(Detail::VertexPC, m_r)));
        functions->glEnableVertexAttribArray(2);
        extra->glVertexAttribIPointer(
            2, 2, GL_UNSIGNED_INT, static_cast<GLsizei>(sizeof(Detail::VertexPC)),
            reinterpret_cast<const void*>(offsetof(Detail::VertexPC, m_pickLow)));

        gpu_bucket.m_vao.release();
        gpu_bucket.m_vbo.release();
        gpu_bucket.m_ebo.release();
    }

    m_gpuReady = true;
    LOG_DEBUG("RenderSceneImpl: GPU bucket resources initialized (3 topology buckets)");
}

void RenderSceneImpl::uploadBuckets(const RenderBucket& bucket, RenderDisplayMode mode) {
    std::array<std::vector<Detail::VertexPC>, 3> vertices_by_topology;
    std::array<std::vector<uint32_t>, 3> indices_by_topology;

    const auto append_pass = [&](const RenderData& pass_data) {
        for(const auto& primitive : pass_data.m_primitives) {
            if(!primitive.m_visible || !Detail::isModeVisible(mode, primitive.m_topology)) {
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
            const auto [pick_low, pick_high_type] =
                Detail::packUidType(primitive.m_entityUID, primitive.m_entityType);

            for(uint32_t i = 0; i < vertex_count; ++i) {
                const size_t offset = static_cast<size_t>(i) * 3;
                vertices.push_back(Detail::VertexPC{
                    primitive.m_positions[offset + 0], primitive.m_positions[offset + 1],
                    primitive.m_positions[offset + 2], primitive.m_color.m_r, primitive.m_color.m_g,
                    primitive.m_color.m_b, primitive.m_color.m_a, pick_low, pick_high_type});
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
                                      static_cast<int>(vertices.size() * sizeof(Detail::VertexPC)));
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
    drawBuckets(mvp, m_shader, m_topologyBuckets);
}

void RenderSceneImpl::drawBuckets(const QMatrix4x4& mvp,
                                  QOpenGLShaderProgram& shader,
                                  std::array<GpuTopologyBucket, 3>& buckets) {
    if(!shader.bind()) {
        LOG_WARN("RenderSceneImpl: failed to bind shader program");
        return;
    }

    auto* context = QOpenGLContext::currentContext();
    auto* functions = context ? context->functions() : nullptr;
    if(!functions) {
        shader.release();
        return;
    }
    shader.setUniformValue("uMvp", mvp);

    functions->glLineWidth(1.5F);

    constexpr std::array<PrimitiveTopology, 3> topology_order = {
        PrimitiveTopology::Points,
        PrimitiveTopology::Lines,
        PrimitiveTopology::Triangles,
    };

    for(int idx = 0; idx < 3; ++idx) {
        auto& gpu_bucket = buckets[idx];
        if(!gpu_bucket.m_created || gpu_bucket.m_indexCount <= 0) {
            continue;
        }

        gpu_bucket.m_vao.bind();
        functions->glDrawElements(Detail::toGlPrimitive(topology_order[idx]),
                                  gpu_bucket.m_indexCount, GL_UNSIGNED_INT, nullptr);
        gpu_bucket.m_vao.release();
    }

    shader.release();
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

    for(auto& gpu_bucket : m_pickTopologyBuckets) {
        if(gpu_bucket.m_created) {
            gpu_bucket.m_vao.destroy();
            gpu_bucket.m_vbo.destroy();
            gpu_bucket.m_ebo.destroy();
            gpu_bucket.m_indexCount = 0;
            gpu_bucket.m_created = false;
        }
    }

    if(m_pickShader.isLinked()) {
        m_pickShader.removeAllShaders();
    }

    m_gpuReady = false;
}

} // namespace OpenGeoLab::Render
