/**
 * @file render_engine.cpp
 * @brief Implements the top-level render engine.
 */
#include <opengeolab/render/render_engine.hpp>

#include <opengeolab/render/grid_pass.hpp>

#include <glad/gl.h>

#include <memory>

namespace OpenGeoLab::Render {

namespace {

[[nodiscard]] float safeAspect(int width, int height) {
    return height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0F;
}

} // namespace

RenderEngine::RenderEngine() = default;

RenderEngine::~RenderEngine() {
    if(initialized_) {
        passManager_.teardownAll();
    }
}

bool RenderEngine::initialize(GlLoaderFunc loader, void* userptr) {
    if(initialized_) {
        return true;
    }

    const int version = gladLoadGLUserPtr(loader, userptr);
    if(version == 0) {
        return false;
    }

    passManager_.registerPass("grid", std::make_unique<GridPass>(), 100);
    passManager_.setupAll(width_, height_);
    initialized_ = true;
    return true;
}

void RenderEngine::resize(int width, int height) {
    width_ = width;
    height_ = height;
    camera_.setAspect(safeAspect(width, height));

    if(initialized_) {
        passManager_.setupAll(width, height);
    }
}

void RenderEngine::render() {
    if(!initialized_) {
        return;
    }

    const CameraState camera_state = camera_.captureState();
    RenderContext ctx;
    ctx.viewportWidth = width_;
    ctx.viewportHeight = height_;
    ctx.viewMatrix = camera_.viewMatrix();
    ctx.projectionMatrix = camera_.projectionMatrix();
    ctx.cameraPosition = camera_.position();
    ctx.clearColor = glm::vec4{0.15F, 0.15F, 0.17F, 1.0F};
    ctx.nearPlane = camera_state.nearPlane;
    ctx.farPlane = camera_state.farPlane;

    glViewport(0, 0, width_, height_);
    glClearColor(ctx.clearColor.r, ctx.clearColor.g, ctx.clearColor.b, ctx.clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    passManager_.executeAll(ctx);
}

Camera& RenderEngine::camera() { return camera_; }

const Camera& RenderEngine::camera() const { return camera_; }

PassManager& RenderEngine::passManager() { return passManager_; }

} // namespace OpenGeoLab::Render
