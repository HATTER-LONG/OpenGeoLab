/**
 * @file render_engine.hpp
 * @brief Declares the top-level render engine combining Camera and PassManager.
 */
#pragma once

#include <opengeolab/render/camera.hpp>
#include <opengeolab/render/pass_manager.hpp>
#include <opengeolab/render/render_export.hpp>

namespace OpenGeoLab::Scene {
class SceneGraph;
}

namespace OpenGeoLab::Render {

class GeometryPass;
class WireframePass;

/** @brief Opaque function pointer representing a GL procedure address. */
using GlFuncPtr = void (*)();

/**
 * @brief Callback that resolves an OpenGL function by name.
 * @param userptr Opaque context forwarded from initialize().
 * @param name Null-terminated GL function name, for example @c "glCreateShader".
 * @return Function pointer, or @c nullptr if not found.
 */
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
    /** @brief Construct a render engine with built-in passes disconnected from any scene. */
    RenderEngine();

    /** @brief Destroy the render engine and tear down owned render passes. */
    ~RenderEngine();

    /**
     * @brief Load GL function pointers and initialize built-in passes.
     * @param loader Callback used to resolve GL function addresses.
     * @param userptr Opaque pointer forwarded to @p loader on each call.
     * @return true on success, false if GL loading failed.
     */
    [[nodiscard]] bool initialize(GlLoaderFunc loader, void* userptr);

    /**
     * @brief Handle viewport resize.
     * @param width New viewport width in pixels.
     * @param height New viewport height in pixels.
     */
    void resize(int width, int height);

    /** @brief Execute one frame. */
    void render();

    /**
     * @brief Set the scene graph to read geometry from.
     * @param graph Pointer to scene graph, or nullptr to disconnect.
     */
    void setSceneGraph(Scene::SceneGraph* graph);

    /** @brief Access the engine camera used for frame rendering. */
    Camera& camera();

    /** @brief Access the engine camera used for frame rendering. */
    const Camera& camera() const;

    /** @brief Access the pass manager coordinating built-in render passes. */
    PassManager& passManager();

private:
    Camera camera_;
    PassManager passManager_;
    GeometryPass* geometryPass_ = nullptr;
    WireframePass* wireframePass_ = nullptr;
    Scene::SceneGraph* sceneGraph_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool sceneDirty_ = true;
    bool initialized_ = false;
};

} // namespace OpenGeoLab::Render
