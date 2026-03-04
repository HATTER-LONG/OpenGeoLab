/**
 * @file draw_batch_utils.hpp
 * @brief Utility helpers to batch DrawRange into OpenGL multi-draw calls.
 */

#pragma once

#include "render/render_data.hpp"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render::PassUtil {

template <typename Predicate>
void buildIndexedBatch(const std::vector<DrawRange>& ranges,
                       Predicate&& predicate,
                       std::vector<GLsizei>& out_counts,
                       std::vector<const void*>& out_offsets) {
    out_counts.clear();
    out_offsets.clear();
    out_counts.reserve(ranges.size());
    out_offsets.reserve(ranges.size());

    for(const auto& range : ranges) {
        if(!predicate(range)) {
            continue;
        }
        if(range.m_indexCount == 0) {
            continue;
        }
        out_counts.push_back(static_cast<GLsizei>(range.m_indexCount));
        out_offsets.push_back(reinterpret_cast<const void*>(
            static_cast<uintptr_t>(range.m_indexOffset) * sizeof(uint32_t)));
    }
}

template <typename Predicate>
void buildArrayBatch(const std::vector<DrawRange>& ranges,
                     Predicate&& predicate,
                     std::vector<GLint>& out_firsts,
                     std::vector<GLsizei>& out_counts) {
    out_firsts.clear();
    out_counts.clear();
    out_firsts.reserve(ranges.size());
    out_counts.reserve(ranges.size());

    for(const auto& range : ranges) {
        if(!predicate(range)) {
            continue;
        }
        if(range.m_vertexCount == 0) {
            continue;
        }
        out_firsts.push_back(static_cast<GLint>(range.m_vertexOffset));
        out_counts.push_back(static_cast<GLsizei>(range.m_vertexCount));
    }
}

inline void multiDrawElements(QOpenGLContext* ctx,
                              QOpenGLFunctions* f,
                              GLenum mode,
                              const std::vector<GLsizei>& counts,
                              const std::vector<const void*>& offsets) {
    if(counts.empty()) {
        return;
    }

    if(auto* f14 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(ctx);
       f14 && f14->initializeOpenGLFunctions()) {
        f14->glMultiDrawElements(mode, counts.data(), GL_UNSIGNED_INT,
                                 reinterpret_cast<const void* const*>(offsets.data()),
                                 static_cast<GLsizei>(counts.size()));
        return;
    }

    for(size_t i = 0; i < counts.size(); ++i) {
        f->glDrawElements(mode, counts[i], GL_UNSIGNED_INT, offsets[i]);
    }
}

inline void multiDrawArrays(QOpenGLContext* ctx,
                            QOpenGLFunctions* f,
                            GLenum mode,
                            const std::vector<GLint>& firsts,
                            const std::vector<GLsizei>& counts) {
    if(counts.empty()) {
        return;
    }

    if(auto* f14 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(ctx);
       f14 && f14->initializeOpenGLFunctions()) {
        f14->glMultiDrawArrays(mode, firsts.data(), counts.data(),
                               static_cast<GLsizei>(counts.size()));
        return;
    }

    for(size_t i = 0; i < counts.size(); ++i) {
        f->glDrawArrays(mode, firsts[i], counts[i]);
    }
}

} // namespace OpenGeoLab::Render::PassUtil
