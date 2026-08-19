/**
 * @file render_pipeline.hpp
 * @brief RenderPipeline — top-level rendering entry point
 */

#pragma once

#include <opengeolab/render/frame_state.hpp>
#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/render/render_export.hpp>

#include <opengeolab/core/entity_ref.hpp>

#include <glm/vec3.hpp>

#include <QtCore/qglobal.h>

#include <memory>
#include <span>
#include <string>
#include <vector>

namespace OpenGeoLab::Scene {
class SceneGraph;
class TopologyIndex;
struct DrawRange;
} // namespace OpenGeoLab::Scene

QT_BEGIN_NAMESPACE
class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;
QT_END_NAMESPACE

namespace OpenGeoLab::Render {

/** @brief Top-level rendering entry point managing the multi-pass pipeline. */
class OPENGEOLAB_RENDER_EXPORT RenderPipeline final {
public:
    RenderPipeline();
    ~RenderPipeline();

    /**
     * @brief Upload scene geometry changes to GPU buffers.
     *
     * Caller must hold SceneGraph read lock. Re-uploads only when the
     * scene version has changed since the last call.
     */
    void synchronize(const Scene::SceneGraph& scene);

    /**
     * @brief Execute the full multi-pass rendering pipeline.
     * @param state Per-frame camera, viewport and display state.
     */
    void render(QRhi* rhi,
                QRhiCommandBuffer* command_buffer,
                QRhiRenderTarget* render_target,
                const FrameState& state);

    /**
     * @brief Resolve a single-pixel pick at the given framebuffer coordinate.
     * @param x Horizontal pixel coordinate (left = 0).
     * @param y Vertical pixel coordinate (top = 0, GL-flipped internally).
     * @param mask Bitmask controlling which entity types are considered.
     * @return Resolved pick result, or an invalid result if nothing was hit.
     */
    [[nodiscard]] PickResult pickAt(int x, int y, PickMask mask) const;

    /**
     * @brief Resolve all picks within a circular region (box-select).
     * @param cx Center x in framebuffer pixels.
     * @param cy Center y in framebuffer pixels.
     * @param radius Search radius in pixels.
     * @param mask Bitmask controlling which entity types are considered.
     * @return All unique resolved results sorted by distance from center.
     */
    [[nodiscard]] std::vector<PickResult>
    pickRegion(int cx, int cy, int radius, PickMask mask) const;

    /**
     * @brief Resolve all picks within an arbitrary rectangle (box-select).
     * @param x0,y0,x1,y1 Rectangle in item-space pixels.
     * @param mask Bitmask controlling which entity types are considered.
     * @return All unique resolved results in the rectangle.
     */
    [[nodiscard]] std::vector<PickResult>
    pickRect(int x0, int y0, int x1, int y1, PickMask mask) const;

    /**
     * @brief Resolve an entity to its DrawRanges via the internal entity index.
     * @return DrawRanges for the entity (may span multiple primitive types). Empty if not found.
     */
    [[nodiscard]] std::span<const Scene::DrawRange> resolveEntityDrawRanges(
        uint32_t shape_id, Core::EntityType entity_type, uint32_t local_id) const;

    /**
     * @brief Collect all DrawRanges belonging to a shape (all topologies).
     *
     * Used for compound-entity expansion (e.g. highlighting all VEF when a solid is selected).
     */
    [[nodiscard]] std::vector<Scene::DrawRange> resolveShapeDrawRanges(uint32_t shape_id) const;

    /**
     * @brief Set the directory containing MSDF atlas resources.
     *
     * Must be called before initialize(). The directory should contain
     * `label_atlas.json` and `label_atlas.png`.
     */
    void setFontAtlasDir(const std::string& dir);

    /**
     * @brief Compute the world-space anchor (centroid) of an entity.
     *
     * Uses GpuBufferManager's CPU-side vertex cache to extract positions
     * and compute the centroid.
     * @return Centroid position, or origin if the entity is not found.
     */
    [[nodiscard]] glm::vec3 resolveEntityAnchor(uint32_t shape_id,
                                                 Core::EntityType entity_type,
                                                 uint32_t local_id) const;

    /** @brief Release all QRhi resources owned by the pipeline. */
    void cleanup();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace OpenGeoLab::Render
