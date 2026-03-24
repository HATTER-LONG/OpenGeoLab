/**
 * @file render_engine.hpp
 * @brief Declares the top-level render engine combining Camera and PassManager.
 */
#pragma once

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/pass_manager.hpp>
#include <opengeolab/render/render_export.hpp>

namespace OpenGeoLab::Render {

/// Opaque function pointer representing a GL procedure address.
using GlFuncPtr = void (*)();

/// Callback that resolves an OpenGL function by name.
/// @param userptr Opaque context forwarded from initialize().
/// @param name    Null-terminated GL function name (e.g. "glCreateShader").
/// @return Function pointer, or nullptr if not found.
using GlLoaderFunc = GlFuncPtr (*)(void* userptr, const char* name);

/**
 * @brief Top-level render engine combining Camera and PassManager.
 *
 * Owned by app's GLViewportItem::Renderer.
 * initialize() must be called after GL context is available.
 * render() is called each frame.
 */
class OPENGEOLAB_RENDER_EXPORT RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    /**
     * @brief Load GL function pointers and initialize built-in passes.
     * @param loader Callback used to resolve GL function addresses.
     * @param userptr Opaque pointer forwarded to @p loader on each call.
     * @return true on success, false if GL loading failed.
     */
    [[nodiscard]] bool initialize(GlLoaderFunc loader, void* userptr);

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
