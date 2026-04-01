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

#include <memory>
#include <span>
#include <vector>

namespace OpenGeoLab::Scene {
class SceneGraph;
class TopologyIndex;
struct DrawRange;
} // namespace OpenGeoLab::Scene

namespace OpenGeoLab::Render {

/**
 * @brief GL function loader — a function pointer compatible with platform getProcAddress.
 *
 * The caller supplies a loader (e.g. wrapping QOpenGLContext::getProcAddress)
 * so the render DLL can populate its own glad function pointers.
 */
using GlLoaderFunc = void* (*)(const char*);

/** @brief Top-level rendering entry point managing the multi-pass pipeline. */
class OPENGEOLAB_RENDER_EXPORT RenderPipeline final {
public:
    RenderPipeline();
    ~RenderPipeline();

    /**
     * @brief Initialize GPU resources and shader programs.
     * @param glLoader Optional GL function loader. When non-null the render
     *        library calls gladLoadGL internally so that its function pointers
     *        are valid (necessary when the render library is a separate DLL).
     */
    void initialize(GlLoaderFunc gl_loader = nullptr);

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
    void render(const FrameState& state);

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

    /** @brief Release all GPU resources owned by the pipeline. */
    void cleanup();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace OpenGeoLab::Render
