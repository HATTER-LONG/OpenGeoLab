/**
 * @file render_engine.hpp
 * @brief Declares the top-level render engine combining Camera and PassManager.
 */
#pragma once

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/pass_manager.hpp>
#include <opengeolab/render/render_export.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief Top-level render engine combining Camera and PassManager.
 *
 * Owned by app/'s GLViewportItem::Renderer.
 * initialize() must be called after GL context is available.
 * render() is called each frame.
 */
class OPENGEOLAB_RENDER_EXPORT RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    /** @brief Initialize built-in passes. Call after GL context is ready. */
    void initialize();

    /** @brief Handle viewport resize. */
    void resize(int width, int height);

    /** @brief Execute one frame. */
    void render();

    Camera& camera();
    const Camera& camera() const;
    PassManager& passManager();

private:
    Camera camera_;
    PassManager passManager_;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
};

} // namespace OpenGeoLab::Render
