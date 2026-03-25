/**
 * @file geometry_pass.cpp
 * @brief Implements the geometry render pass with Phong shading.
 */
#include <opengeolab/render/geometry_pass.hpp>

#include <glad/gl.h>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

#include <string_view>
#include <utility>

namespace OpenGeoLab::Render {

namespace {

constexpr const char* kGeometryVertexShader = R"(#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normalMatrix;

out vec3 vNormal;
out vec3 vFragPos;

void main() {
    vec4 worldPosition = u_model * vec4(aPosition, 1.0);
    mat3 viewRotation = transpose(mat3(u_view));

    vFragPos = worldPosition.xyz;
    vNormal = normalize(viewRotation * u_normalMatrix * aNormal);
    gl_Position = u_projection * u_view * worldPosition;
}
)";

constexpr const char* kGeometryFragmentShader = R"(#version 450 core

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 u_viewPos;
uniform vec3 u_lightDir;
uniform vec3 u_objectColor;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(u_lightDir);
    vec3 viewDir = normalize(u_viewPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    vec3 ambient = 0.15 * u_objectColor;
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * u_objectColor;
    vec3 specular = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * vec3(0.3);

    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";

constexpr glm::vec3 kObjectColor{0.6F, 0.7F, 0.8F};

void setMat3Uniform(std::uint32_t program_id, std::string_view name, const glm::mat3& value) {
    const std::string uniform_name{name};
    glUniformMatrix3fv(glGetUniformLocation(program_id, uniform_name.c_str()), 1, GL_FALSE,
                       glm::value_ptr(value));
}

} // namespace

void GeometryPass::setup(int /*width*/, int /*height*/) {
    if(initialized_) {
        return;
    }

    if(!shader_.compile(kGeometryVertexShader, kGeometryFragmentShader)) {
        return;
    }

    initialized_ = true;
}

void GeometryPass::execute(const RenderContext& ctx) {
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
            if(entry.faceMesh.topology == Scene::PrimitiveType::Triangles &&
               !entry.faceMesh.positions.empty() && !entry.faceMesh.indices.empty()) {
                vao.upload(entry.faceMesh);
            }
            vaos_.push_back(std::move(vao));
        }

        dirty_ = false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    const glm::vec3 light_direction = glm::normalize(
        ctx.cameraPosition == glm::vec3{0.0F} ? glm::vec3{0.0F, 0.0F, 1.0F} : ctx.cameraPosition);

    for(std::size_t index = 0; index < entries_.size() && index < vaos_.size(); ++index) {
        const auto& entry = entries_[index];
        const auto& vao = vaos_[index];
        const glm::mat4 model_view = ctx.viewMatrix * entry.transform;
        const glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(model_view)));

        shader_.use();
        shader_.setMat4("u_model", entry.transform);
        shader_.setMat4("u_view", ctx.viewMatrix);
        shader_.setMat4("u_projection", ctx.projectionMatrix);
        setMat3Uniform(shader_.id(), "u_normalMatrix", normal_matrix);
        shader_.setVec3("u_viewPos", ctx.cameraPosition);
        shader_.setVec3("u_lightDir", light_direction);
        shader_.setVec3("u_objectColor", kObjectColor);
        vao.draw();
    }
}

void GeometryPass::teardown() {
    for(auto& vao : vaos_) {
        vao.release();
    }

    vaos_.clear();
    shader_ = ShaderProgram{};
    initialized_ = false;
    dirty_ = !entries_.empty();
}

void GeometryPass::setGeometry(std::vector<Entry> entries) {
    entries_ = std::move(entries);
    dirty_ = true;
}

} // namespace OpenGeoLab::Render
