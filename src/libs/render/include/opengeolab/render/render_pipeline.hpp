/**
 * @file render_pipeline.hpp
 * @brief RenderPipeline — top-level rendering entry point
 */

#pragma once

#include <opengeolab/render/frame_state.hpp>
#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/render/render_export.hpp>

#include <memory>
#include <vector>

namespace OpenGeoLab::Scene {
class SceneGraph;
class TopologyIndex;
} // namespace OpenGeoLab::Scene

namespace OpenGeoLab::Render {

/**
 * @brief GL function loader — a function pointer compatible with platform getProcAddress.
 *
 * The caller supplies a loader (e.g. wrapping QOpenGLContext::getProcAddress)
 * so the render DLL can populate its own glad function pointers.
 */
using GlLoaderFunc = void* (*)(const char*);

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
    void initialize(GlLoaderFunc glLoader = nullptr);

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
    [[nodiscard]] std::vector<PickResult> pickRegion(int cx, int cy, int radius,
                                                     PickMask mask) const;

    /** @brief Release all GPU resources owned by the pipeline. */
    void cleanup();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace OpenGeoLab::Render
