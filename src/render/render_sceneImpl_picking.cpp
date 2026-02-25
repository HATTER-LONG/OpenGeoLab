/**
 * @file render_sceneImpl_picking.cpp
 * @brief Picking pipeline implementation for RenderSceneImpl
 */

#include "render_sceneImpl.hpp"

#include "render/render_scene_controller.hpp"
#include "render/render_select_manager.hpp"
#include "render_scene_impl_internal.hpp"
#include "util/logger.hpp"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace OpenGeoLab::Render {

void RenderSceneImpl::processPicking(const PickingInput& input) {
    if(input.m_action == PickAction::None) {
        return;
    }

    auto& select_manager = RenderSelectManager::instance();
    if(!select_manager.isPickEnabled()) {
        return;
    }

    auto* context = QOpenGLContext::currentContext();
    if(!context) {
        return;
    }

    auto* gl = context->functions();
    if(!gl) {
        return;
    }

    if(!ensurePickResources()) {
        LOG_WARN("RenderSceneImpl: pick resources are not ready");
        return;
    }

    const int width = std::max(
        1, static_cast<int>(std::round(input.m_itemSize.width() * input.m_devicePixelRatio)));
    const int height = std::max(
        1, static_cast<int>(std::round(input.m_itemSize.height() * input.m_devicePixelRatio)));
    if(!ensurePickFramebuffer(QSize(width, height))) {
        LOG_WARN("RenderSceneImpl: pick framebuffer creation failed");
        return;
    }

    const RenderEntityTypeMask pick_mask = select_manager.getPickTypes();
    if(pick_mask == RenderEntityTypeMask::None) {
        return;
    }

    std::array<std::vector<Detail::VertexPC>, 3> vertices_by_topology;
    std::array<std::vector<uint32_t>, 3> indices_by_topology;

    const auto& bucket = RenderSceneController::instance().renderData();
    const auto append_pass = [&](const RenderData& pass_data) {
        for(const auto& primitive : pass_data.m_primitives) {
            if(!primitive.m_visible || !Detail::isPickableType(primitive.m_entityType, pick_mask)) {
                continue;
            }

            if((primitive.m_positions.size() % 3) != 0) {
                continue;
            }

            const auto [pick_low, pick_high_type] =
                Detail::packUidType(primitive.m_entityUID, primitive.m_entityType);

            const int topo_idx = topologyIndex(primitive.m_topology);
            auto& vertices = vertices_by_topology[topo_idx];
            auto& indices = indices_by_topology[topo_idx];
            const uint32_t base_vertex = static_cast<uint32_t>(vertices.size());
            const uint32_t vertex_count = static_cast<uint32_t>(primitive.m_positions.size() / 3);

            vertices.reserve(vertices.size() + vertex_count);
            for(uint32_t i = 0; i < vertex_count; ++i) {
                const size_t offset = static_cast<size_t>(i) * 3;
                vertices.push_back(Detail::VertexPC{
                    primitive.m_positions[offset], primitive.m_positions[offset + 1],
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
        auto& gpu_bucket = m_pickTopologyBuckets[idx];
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

    GLint previous_framebuffer = 0;
    gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_framebuffer);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, m_pickFramebuffer);
    gl->glViewport(0, 0, width, height);
    gl->glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const QMatrix4x4 mvp = input.m_projectionMatrix * input.m_viewMatrix;
    drawBuckets(mvp, m_pickShader, m_pickTopologyBuckets);

    int px = static_cast<int>(std::round(input.m_cursorPos.x() * input.m_devicePixelRatio));
    int py = static_cast<int>(std::round(input.m_cursorPos.y() * input.m_devicePixelRatio));
    px = std::clamp(px, 0, width - 1);
    py = std::clamp(py, 0, height - 1);
    const int gl_py = std::clamp(height - 1 - py, 0, height - 1);

    std::array<uint32_t, 2> picked = {0U, 0U};
    gl->glReadPixels(px, gl_py, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, picked.data());
    gl->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previous_framebuffer));

    const PickResult result = Detail::unpackPick(picked[0], picked[1]);
    if(result.m_uid == 0 || result.m_type == RenderEntityType::None ||
       !Detail::isPickableType(result.m_type, pick_mask)) {
        return;
    }

    if(input.m_action == PickAction::Add) {
        select_manager.addSelection(result.m_uid, result.m_type);
    } else if(input.m_action == PickAction::Remove) {
        select_manager.removeSelection(result.m_uid, result.m_type);
    }
}

bool RenderSceneImpl::ensurePickResources() {
    auto* context = QOpenGLContext::currentContext();
    if(!context) {
        return false;
    }

    auto* functions = context->functions();
    auto* extra = context->extraFunctions();
    if(!functions) {
        return false;
    }
    if(!extra) {
        return false;
    }

    if(m_pickShader.isLinked() && m_pickTopologyBuckets[0].m_created &&
       m_pickTopologyBuckets[1].m_created && m_pickTopologyBuckets[2].m_created) {
        return true;
    }

    if(!m_pickShader.isLinked()) {
        if(!m_pickShader.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                                 R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 2) in uvec2 aPick;
            uniform mat4 uMvp;
            flat out uvec2 vPick;
            void main() {
                gl_Position = uMvp * vec4(aPos, 1.0);
                vPick = aPick;
                gl_PointSize = 6.0;
            }
        )")) {
            LOG_ERROR("RenderSceneImpl: pick vertex shader compile failed: {}",
                      m_pickShader.log().toStdString());
            return false;
        }

        if(!m_pickShader.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                 R"(
            #version 330 core
            flat in uvec2 vPick;
            layout(location = 0) out uvec2 outPick;
            void main() {
                outPick = vPick;
            }
        )")) {
            LOG_ERROR("RenderSceneImpl: pick fragment shader compile failed: {}",
                      m_pickShader.log().toStdString());
            return false;
        }

        if(!m_pickShader.link()) {
            LOG_ERROR("RenderSceneImpl: pick shader link failed: {}",
                      m_pickShader.log().toStdString());
            return false;
        }
    }

    for(auto& gpu_bucket : m_pickTopologyBuckets) {
        if(gpu_bucket.m_created) {
            continue;
        }

        gpu_bucket.m_created =
            gpu_bucket.m_vao.create() && gpu_bucket.m_vbo.create() && gpu_bucket.m_ebo.create();
        if(!gpu_bucket.m_created) {
            LOG_ERROR("RenderSceneImpl: failed to create picking GPU bucket resources");
            return false;
        }

        gpu_bucket.m_vao.bind();
        gpu_bucket.m_vbo.bind();
        gpu_bucket.m_ebo.bind();

        functions->glEnableVertexAttribArray(0);
        functions->glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(Detail::VertexPC)),
            reinterpret_cast<const void*>(offsetof(Detail::VertexPC, m_x)));
        functions->glEnableVertexAttribArray(2);
        extra->glVertexAttribIPointer(
            2, 2, GL_UNSIGNED_INT, static_cast<GLsizei>(sizeof(Detail::VertexPC)),
            reinterpret_cast<const void*>(offsetof(Detail::VertexPC, m_pickLow)));

        gpu_bucket.m_vao.release();
        gpu_bucket.m_vbo.release();
        gpu_bucket.m_ebo.release();
    }

    return true;
}

bool RenderSceneImpl::ensurePickFramebuffer(const QSize& size) {
    auto* context = QOpenGLContext::currentContext();
    auto* gl = context ? context->functions() : nullptr;
    auto* extra = context ? context->extraFunctions() : nullptr;
    if(!gl || !extra) {
        return false;
    }

    if(size.width() <= 0 || size.height() <= 0) {
        return false;
    }

    if(m_pickFramebuffer != 0 && m_pickBufferSize == size) {
        return true;
    }

    releasePickFramebuffer();

    gl->glGenFramebuffers(1, &m_pickFramebuffer);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, m_pickFramebuffer);

    gl->glGenTextures(1, &m_pickColorTexture);
    gl->glBindTexture(GL_TEXTURE_2D, m_pickColorTexture);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, size.width(), size.height(), 0, GL_RG_INTEGER,
                     GL_UNSIGNED_INT, nullptr);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               m_pickColorTexture, 0);

    gl->glGenRenderbuffers(1, &m_pickDepthRenderbuffer);
    gl->glBindRenderbuffer(GL_RENDERBUFFER, m_pickDepthRenderbuffer);
    gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.width(), size.height());
    gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                  m_pickDepthRenderbuffer);

    const GLenum attachments[] = {GL_COLOR_ATTACHMENT0};
    extra->glDrawBuffers(1, attachments);

    const bool complete = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if(!complete) {
        releasePickFramebuffer();
        return false;
    }

    m_pickBufferSize = size;
    return true;
}

void RenderSceneImpl::releasePickFramebuffer() {
    auto* context = QOpenGLContext::currentContext();
    auto* gl = context ? context->functions() : nullptr;
    if(!gl) {
        m_pickFramebuffer = 0;
        m_pickColorTexture = 0;
        m_pickDepthRenderbuffer = 0;
        m_pickBufferSize = QSize();
        return;
    }

    if(m_pickColorTexture != 0) {
        gl->glDeleteTextures(1, &m_pickColorTexture);
        m_pickColorTexture = 0;
    }
    if(m_pickDepthRenderbuffer != 0) {
        gl->glDeleteRenderbuffers(1, &m_pickDepthRenderbuffer);
        m_pickDepthRenderbuffer = 0;
    }
    if(m_pickFramebuffer != 0) {
        gl->glDeleteFramebuffers(1, &m_pickFramebuffer);
        m_pickFramebuffer = 0;
    }
    m_pickBufferSize = QSize();
}

} // namespace OpenGeoLab::Render
