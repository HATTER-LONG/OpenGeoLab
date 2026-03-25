/**
 * @file wireframe_pass.cpp
 * @brief Implements the wireframe edge overlay render pass.
 */
#include <opengeolab/render/wireframe_pass.hpp>

#include <glad/gl.h>
#include <glm/vec3.hpp>

#include <utility>

namespace OpenGeoLab::Render {

namespace {

constexpr const char* kWireframeVertexShader = R"(#version 450 core

layout(location = 0) in vec3 aPosition;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * u_view * u_model * vec4(aPosition, 1.0);
}
)";

constexpr const char* kWireframeFragmentShader = R"(#version 450 core

uniform vec3 u_lineColor;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(u_lineColor, 1.0);
}
)";

constexpr glm::vec3 kLineColor{0.1F, 0.1F, 0.1F};

} // namespace

void WireframePass::setup(int /*width*/, int /*height*/) {
    if(initialized_) {
        return;
    }

    if(!shader_.compile(kWireframeVertexShader, kWireframeFragmentShader)) {
        return;
    }

    initialized_ = true;
}

void WireframePass::execute(const RenderContext& ctx) {
    if(!initialized_) {
        return;
    }

    if(dirty_) {
        for(auto& vao : vaos_) {
            vao.release();
        }

        vaos_.clear();
        vaos_.reserve(entries_.size());
        for(const auto& entry : entries_) {
            VertexArrayObject vao;
            if(entry.edgeMesh.topology == Scene::PrimitiveType::Lines &&
               !entry.edgeMesh.positions.empty() && !entry.edgeMesh.indices.empty()) {
                vao.upload(entry.edgeMesh);
            }
            vaos_.push_back(std::move(vao));
        }

        dirty_ = false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0F, -1.0F);
    glLineWidth(1.5F);

    for(std::size_t index = 0; index < entries_.size() && index < vaos_.size(); ++index) {
        const auto& entry = entries_[index];
        const auto& vao = vaos_[index];

        shader_.use();
        shader_.setMat4("u_model", entry.transform);
        shader_.setMat4("u_view", ctx.viewMatrix);
        shader_.setMat4("u_projection", ctx.projectionMatrix);
        shader_.setVec3("u_lineColor", kLineColor);
        vao.draw();
    }

    glDisable(GL_POLYGON_OFFSET_LINE);
}

void WireframePass::teardown() {
    for(auto& vao : vaos_) {
        vao.release();
    }

    vaos_.clear();
    shader_ = ShaderProgram{};
    initialized_ = false;
    dirty_ = !entries_.empty();
}

void WireframePass::setGeometry(std::vector<Entry> entries) {
    entries_ = std::move(entries);
    dirty_ = true;
}

} // namespace OpenGeoLab::Render
