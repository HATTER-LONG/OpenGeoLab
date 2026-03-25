/**
 * @file render_engine.cpp
 * @brief Implements the top-level render engine.
 */
#include <opengeolab/render/render_engine.hpp>

#include <opengeolab/render/geometry_pass.hpp>
#include <opengeolab/render/grid_pass.hpp>
#include <opengeolab/render/wireframe_pass.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <glad/gl.h>

#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

namespace {

[[nodiscard]] float safeAspect(int width, int height) {
    return height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0F;
}

} // namespace

RenderEngine::RenderEngine() = default;

RenderEngine::~RenderEngine() {
    setSceneGraph(nullptr);
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
    auto geometry_pass = std::make_unique<GeometryPass>();
    auto wireframe_pass = std::make_unique<WireframePass>();
    geometryPass_ = geometry_pass.get();
    wireframePass_ = wireframe_pass.get();
    passManager_.registerPass("geometry", std::move(geometry_pass), 200);
    passManager_.registerPass("wireframe", std::move(wireframe_pass), 300);
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

void RenderEngine::setSceneGraph(Scene::SceneGraph* graph) {
    if(sceneGraph_ != nullptr) {
        sceneGraph_->onChanged = {};
    }

    sceneGraph_ = graph;
    sceneDirty_ = true;
    if(sceneGraph_ != nullptr) {
        sceneGraph_->onChanged = [this]() { sceneDirty_ = true; };
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

    if(sceneDirty_) {
        std::vector<GeometryPass::Entry> geometry_entries;
        std::vector<WireframePass::Entry> wireframe_entries;

        if(sceneGraph_ != nullptr) {
            for(const auto& child : sceneGraph_->root().children) {
                for(const auto& mesh : child.meshes) {
                    if(mesh.topology == Scene::PrimitiveType::Triangles) {
                        geometry_entries.push_back({mesh, child.transform});
                    } else if(mesh.topology == Scene::PrimitiveType::Lines) {
                        wireframe_entries.push_back({mesh, child.transform});
                    }
                }
            }
        }

        if(geometryPass_ != nullptr) {
            geometryPass_->setGeometry(std::move(geometry_entries));
        }

        if(wireframePass_ != nullptr) {
            wireframePass_->setGeometry(std::move(wireframe_entries));
        }

        sceneDirty_ = false;
    }

    passManager_.executeAll(ctx);
}

Camera& RenderEngine::camera() { return camera_; }

const Camera& RenderEngine::camera() const { return camera_; }

PassManager& RenderEngine::passManager() { return passManager_; }

} // namespace OpenGeoLab::Render
