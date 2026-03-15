/**
 * @file RenderFrame.hpp
 * @brief Render-frame types used to validate render-layer data flow.
 */

#pragma once

#include <ogl/render/export.hpp>
#include <ogl/scene/SceneGraph.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace OGL::Render {

/**
 * @brief Minimal camera state carried by the render frame.
 */
struct OGL_RENDER_EXPORT CameraPose {
    double yawDegrees{32.0};
    double pitchDegrees{-18.0};
    double distance{8.5};

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Single renderable item mapped from a scene node.
 */
struct OGL_RENDER_EXPORT DrawItem {
    std::string nodeId;
    std::string pipelineKey;
    std::string colorHex;
    bool highlighted{false};

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Lightweight render-frame description produced from scene-layer data.
 */
class OGL_RENDER_EXPORT RenderFrame {
public:
    RenderFrame(std::string frame_id,
                std::string scene_id,
                int viewport_width,
                int viewport_height,
                CameraPose camera,
                std::vector<DrawItem> draw_items);

    [[nodiscard]] auto frameId() const -> const std::string&;
    [[nodiscard]] auto sceneId() const -> const std::string&;
    [[nodiscard]] auto viewportWidth() const -> int;
    [[nodiscard]] auto viewportHeight() const -> int;
    [[nodiscard]] auto camera() const -> const CameraPose&;
    [[nodiscard]] auto drawItems() const -> const std::vector<DrawItem>&;
    [[nodiscard]] auto summary() const -> std::string;
    [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
    std::string m_frameId;
    std::string m_sceneId;
    int m_viewportWidth{0};
    int m_viewportHeight{0};
    CameraPose m_camera;
    std::vector<DrawItem> m_drawItems;
};

/**
 * @brief Convert a scene graph into render-frame state.
 * @param scene_graph Scene-layer graph.
 * @param params Optional request parameters such as viewport size and highlight target.
 * @return Render-frame description.
 */
OGL_RENDER_EXPORT auto buildRenderFrame(const OGL::Scene::SceneGraph& scene_graph,
                                        const nlohmann::json& params) -> RenderFrame;

} // namespace OGL::Render
