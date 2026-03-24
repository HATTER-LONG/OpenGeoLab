/**
 * @file render_context.hpp
 * @brief Declares the per-frame read-only render context passed to each Pass.
 */
#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace OpenGeoLab::Render {

/**
 * @brief Per-frame read-only context passed to each render pass.
 */
struct RenderContext {
    int viewportWidth = 0;
    int viewportHeight = 0;
    glm::mat4 viewMatrix{1.0F};
    glm::mat4 projectionMatrix{1.0F};
    glm::vec3 cameraPosition{0.0F};
    glm::vec4 clearColor{0.15F, 0.15F, 0.17F, 1.0F};
    float nearPlane = 0.1F;
    float farPlane = 1000.0F;
};

} // namespace OpenGeoLab::Render