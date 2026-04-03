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

/** @brief Pre-built glMultiDrawElements parameter arrays. */
struct IndexedBatch {
    std::vector<GLsizei> counts;
    std::vector<const void*> offsets;
    [[nodiscard]] GLsizei drawCount() const { return static_cast<GLsizei>(counts.size()); }
};

/** @brief Pre-built glMultiDrawArrays parameter arrays. */
struct ArrayBatch {
    std::vector<GLint> firsts;
    std::vector<GLsizei> counts;
    [[nodiscard]] GLsizei drawCount() const { return static_cast<GLsizei>(counts.size()); }
};

/**
 * @brief Build an indexed batch from DrawRanges matching a predicate.
 * @param predicate Callable `bool(const DrawRange&)` selecting which ranges to include.
 */
template <typename Predicate>
IndexedBatch buildIndexedBatch(const std::vector<Scene::DrawRange>& ranges, Predicate&& predicate) {
    IndexedBatch batch;
    for(const auto& r : ranges) {
        if(predicate(r)) {
            batch.counts.push_back(static_cast<GLsizei>(r.indexCount));
            batch.offsets.push_back(reinterpret_cast<const void*>(
                static_cast<uintptr_t>(r.indexOffset) * sizeof(uint32_t)));
        }
    }
    return batch;
}

/**
 * @brief Build an array batch from DrawRanges matching a predicate.
 * @param predicate Callable `bool(const DrawRange&)` selecting which ranges to include.
 */
template <typename Predicate>
ArrayBatch buildArrayBatch(const std::vector<Scene::DrawRange>& ranges, Predicate&& predicate) {
    ArrayBatch batch;
    for(const auto& r : ranges) {
        if(predicate(r)) {
            batch.firsts.push_back(static_cast<GLint>(r.vertexOffset));
            batch.counts.push_back(static_cast<GLsizei>(r.vertexCount));
        }
    }
    return batch;
}

/** @brief Issue glMultiDrawElements for the given indexed batch. */
OPENGEOLAB_RENDER_EXPORT void multiDrawElements(GLenum mode, const IndexedBatch& batch);

/** @brief Issue glMultiDrawArrays for the given array batch. */
OPENGEOLAB_RENDER_EXPORT void multiDrawArrays(GLenum mode, const ArrayBatch& batch);

/**
 * @brief Filter draw ranges matching a predicate into a new vector.
 * @param predicate Callable `bool(const DrawRange&)` selecting which ranges to include.
 */
template <typename Predicate>
std::vector<Scene::DrawRange> filterRanges(const std::vector<Scene::DrawRange>& ranges,
                                           Predicate&& predicate) {
    std::vector<Scene::DrawRange> result;
    for(const auto& r : ranges) {
        if(predicate(r)) {
            result.push_back(r);
        }
    }
    return result;
}

} // namespace OpenGeoLab::Render::BatchUtils
