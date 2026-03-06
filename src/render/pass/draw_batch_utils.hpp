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
                              const GLsizei* counts,
                              GLenum index_type,
                              const void* const* offsets,
                              GLsizei draw_count) {
    if(draw_count <= 0) {
        return;
    }

    if(auto* f14 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(ctx);
       f14 && f14->initializeOpenGLFunctions()) {
        f14->glMultiDrawElements(mode, counts, index_type, offsets, draw_count);
        return;
    }

    for(GLsizei i = 0; i < draw_count; ++i) {
        f->glDrawElements(mode, counts[i], index_type, offsets[i]);
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

    multiDrawElements(ctx, f, mode, counts.data(), GL_UNSIGNED_INT,
                      reinterpret_cast<const void* const*>(offsets.data()),
                      static_cast<GLsizei>(counts.size()));
}

inline void multiDrawElements(QOpenGLContext* ctx,
                              QOpenGLFunctions* f,
                              GLenum mode,
                              const IndexedDrawBatch& batch) {
    if(batch.empty()) {
        return;
    }

    static_assert(sizeof(GLsizei) == sizeof(int32_t));
    multiDrawElements(ctx, f, mode, reinterpret_cast<const GLsizei*>(batch.m_counts.data()),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<const void* const*>(batch.m_byteOffsets.data()),
                      static_cast<GLsizei>(batch.m_counts.size()));
}

inline void multiDrawArrays(QOpenGLContext* ctx,
                            QOpenGLFunctions* f,
                            GLenum mode,
                            const GLint* firsts,
                            const GLsizei* counts,
                            GLsizei draw_count) {
    if(draw_count <= 0) {
        return;
    }

    if(auto* f14 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(ctx);
       f14 && f14->initializeOpenGLFunctions()) {
        f14->glMultiDrawArrays(mode, firsts, counts, draw_count);
        return;
    }

    for(GLsizei i = 0; i < draw_count; ++i) {
        f->glDrawArrays(mode, firsts[i], counts[i]);
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

    multiDrawArrays(ctx, f, mode, firsts.data(), counts.data(),
                    static_cast<GLsizei>(counts.size()));
}

inline void multiDrawArrays(QOpenGLContext* ctx,
                            QOpenGLFunctions* f,
                            GLenum mode,
                            const ArrayDrawBatch& batch) {
    if(batch.empty()) {
        return;
    }

    static_assert(sizeof(GLint) == sizeof(int32_t));
    static_assert(sizeof(GLsizei) == sizeof(int32_t));
    multiDrawArrays(ctx, f, mode, reinterpret_cast<const GLint*>(batch.m_firsts.data()),
                    reinterpret_cast<const GLsizei*>(batch.m_counts.data()),
                    static_cast<GLsizei>(batch.m_counts.size()));
}

} // namespace OpenGeoLab::Render::PassUtil
