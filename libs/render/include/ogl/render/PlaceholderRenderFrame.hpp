/**
 * @file PlaceholderRenderFrame.hpp
 * @brief Placeholder render-frame types used to validate render-layer data flow.
 */

#pragma once

#include <ogl/render/export.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ogl::render {

/**
 * @brief Minimal camera state carried by the placeholder render frame.
 */
struct OGL_RENDER_EXPORT PlaceholderCameraPose {
    double yawDegrees{32.0};
    double pitchDegrees{-18.0};
    double distance{8.5};

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Single renderable item mapped from a placeholder scene node.
 */
struct OGL_RENDER_EXPORT PlaceholderDrawItem {
    std::string nodeId;
    std::string pipelineKey;
    std::string colorHex;
    bool highlighted{false};

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Lightweight render-frame description produced from scene-layer data.
 */
class OGL_RENDER_EXPORT PlaceholderRenderFrame {
public:
    PlaceholderRenderFrame(std::string frame_id, std::string scene_id, int viewport_width,
                           int viewport_height, PlaceholderCameraPose camera,
                           std::vector<PlaceholderDrawItem> draw_items);

    [[nodiscard]] auto frameId() const -> const std::string&;
    [[nodiscard]] auto sceneId() const -> const std::string&;
    [[nodiscard]] auto viewportWidth() const -> int;
    [[nodiscard]] auto viewportHeight() const -> int;
    [[nodiscard]] auto camera() const -> const PlaceholderCameraPose&;
    [[nodiscard]] auto drawItems() const -> const std::vector<PlaceholderDrawItem>&;
    [[nodiscard]] auto summary() const -> std::string;
    [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
    std::string m_frameId;
    std::string m_sceneId;
    int m_viewportWidth{0};
    int m_viewportHeight{0};
    PlaceholderCameraPose m_camera;
    std::vector<PlaceholderDrawItem> m_drawItems;
};

/**
 * @brief Convert a placeholder scene graph into placeholder render-frame state.
 * @param scene_graph Scene-layer placeholder graph.
 * @param params Optional request parameters such as viewport size and highlight target.
 * @return Placeholder render-frame description.
 */
OGL_RENDER_EXPORT auto buildPlaceholderRenderFrame(const ogl::scene::PlaceholderSceneGraph& scene_graph,
                                                   const nlohmann::json& params)
    -> PlaceholderRenderFrame;

} // namespace ogl::render