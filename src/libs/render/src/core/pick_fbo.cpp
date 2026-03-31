#include "core/pick_fbo.hpp"

#include <opengeolab/core/logger.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace OpenGeoLab::Render {

bool PickFbo::initialize(int width, int height) {
    m_width = width;
    m_height = height;

    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_colorTex);
    glGenRenderbuffers(1, &m_depthRbo);

    createAttachments();

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if(status != GL_FRAMEBUFFER_COMPLETE) {
        Core::getLogger()->error("PickFbo: framebuffer incomplete (status=0x{:X})", status);
        return false;
    }
    return true;
}

void PickFbo::createAttachments() {
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, m_width, m_height, 0, GL_RG_INTEGER, GL_UNSIGNED_INT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void PickFbo::resize(int width, int height) {
    if(width == m_width && height == m_height) {
        return;
    }
    m_width = width;
    m_height = height;
    createAttachments();

    // Re-attach in case drivers need it
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PickFbo::cleanup() {
    if(m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if(m_colorTex != 0) {
        glDeleteTextures(1, &m_colorTex);
        m_colorTex = 0;
    }
    if(m_depthRbo != 0) {
        glDeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }
}

void PickFbo::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void PickFbo::unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

uint64_t PickFbo::readPickId(int x, int y) const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    int const gl_y = m_height - 1 - y;
    uint32_t pixel[2]{};
    glReadPixels(x, gl_y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, pixel);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return (static_cast<uint64_t>(pixel[1]) << 32U) | pixel[0];
}

std::vector<uint64_t> PickFbo::readPickRegion(int cx, int cy, int radius) const {
    int const x0 = std::max(cx - radius, 0);
    int const y0 = std::max((m_height - 1 - cy) - radius, 0);
    int const x1 = std::min(cx + radius, m_width - 1);
    int const y1 = std::min((m_height - 1 - cy) + radius, m_height - 1);
    int const w = x1 - x0 + 1;
    int const h = y1 - y0 + 1;
    if(w <= 0 || h <= 0) {
        return {};
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    std::vector<uint32_t> data(static_cast<size_t>(w * h * 2));
    glReadPixels(x0, y0, w, h, GL_RG_INTEGER, GL_UNSIGNED_INT, data.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    int const pixel_count = w * h;
    int const center_x = cx - x0;
    int const center_y = (m_height - 1 - cy) - y0;

    std::vector<std::pair<int, int>> order;
    order.reserve(static_cast<size_t>(pixel_count));
    for(int i = 0; i < pixel_count; ++i) {
        int const px = i % w;
        int const py = i / w;
        int const dx = px - center_x;
        int const dy = py - center_y;
        order.emplace_back(dx * dx + dy * dy, i);
    }
    std::sort(order.begin(), order.end());

    std::vector<uint64_t> result;
    for(auto [dist2, idx] : order) {
        uint32_t const lo = data[static_cast<size_t>(idx) * 2];
        uint32_t const hi = data[static_cast<size_t>(idx) * 2 + 1];
        uint64_t const pick_id = (static_cast<uint64_t>(hi) << 32U) | lo;
        if(pick_id != 0) {
            result.push_back(pick_id);
        }
    }
    return result;
}

} // namespace OpenGeoLab::Render
