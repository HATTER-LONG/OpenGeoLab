/**
 * @file batch_utils.hpp
 * @brief Batch building utilities for glMultiDraw* calls
 */

#pragma once

#include <opengeolab/render/render_export.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <glad/gl.h>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render::BatchUtils {

struct IndexedBatch {
    std::vector<GLsizei> counts;
    std::vector<const void*> offsets;
    [[nodiscard]] GLsizei drawCount() const {
        return static_cast<GLsizei>(counts.size());
    }
};

struct ArrayBatch {
    std::vector<GLint> firsts;
    std::vector<GLsizei> counts;
    [[nodiscard]] GLsizei drawCount() const {
        return static_cast<GLsizei>(counts.size());
    }
};

template <typename Predicate>
IndexedBatch buildIndexedBatch(const std::vector<Scene::DrawRange>& ranges,
                               Predicate&& predicate) {
    IndexedBatch batch;
    for (const auto& r : ranges) {
        if (predicate(r)) {
            batch.counts.push_back(static_cast<GLsizei>(r.indexCount));
            batch.offsets.push_back(
                reinterpret_cast<const void*>(
                    static_cast<uintptr_t>(r.indexOffset) * sizeof(uint32_t)));
        }
    }
    return batch;
}

template <typename Predicate>
ArrayBatch buildArrayBatch(const std::vector<Scene::DrawRange>& ranges,
                           Predicate&& predicate) {
    ArrayBatch batch;
    for (const auto& r : ranges) {
        if (predicate(r)) {
            batch.firsts.push_back(static_cast<GLint>(r.vertexOffset));
            batch.counts.push_back(static_cast<GLsizei>(r.vertexCount));
        }
    }
    return batch;
}

OPENGEOLAB_RENDER_EXPORT void multiDrawElements(GLenum mode, const IndexedBatch& batch);
OPENGEOLAB_RENDER_EXPORT void multiDrawArrays(GLenum mode, const ArrayBatch& batch);

} // namespace OpenGeoLab::Render::BatchUtils
