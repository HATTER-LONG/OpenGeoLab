#include <ogl/render/RenderFrame.hpp>

#include <algorithm>
#include <array>
#include <sstream>
#include <utility>

namespace {

constexpr std::array<const char*, 4> K_RENDER_PALETTE = {"#4f7b6b", "#b9854c", "#7089a3",
                                                         "#7f5d86"};

} // namespace

namespace OGL::Render {

auto CameraPose::toJson() const -> nlohmann::json {
    return {{"yawDegrees", yawDegrees}, {"pitchDegrees", pitchDegrees}, {"distance", distance}};
}

auto DrawItem::toJson() const -> nlohmann::json {
    return {{"nodeId", nodeId},
            {"pipelineKey", pipelineKey},
            {"colorHex", colorHex},
            {"highlighted", highlighted}};
}

RenderFrame::RenderFrame(std::string frame_id,
                         std::string scene_id,
                         int viewport_width,
                         int viewport_height,
                         CameraPose camera,
                         std::vector<DrawItem> draw_items)
    : m_frameId(std::move(frame_id)), m_sceneId(std::move(scene_id)),
      m_viewportWidth(viewport_width), m_viewportHeight(viewport_height),
      m_camera(std::move(camera)), m_drawItems(std::move(draw_items)) {}

auto RenderFrame::frameId() const -> const std::string& { return m_frameId; }

auto RenderFrame::sceneId() const -> const std::string& { return m_sceneId; }

auto RenderFrame::viewportWidth() const -> int { return m_viewportWidth; }

auto RenderFrame::viewportHeight() const -> int { return m_viewportHeight; }

auto RenderFrame::camera() const -> const CameraPose& { return m_camera; }

auto RenderFrame::drawItems() const -> const std::vector<DrawItem>& {
    return m_drawItems;
}

auto RenderFrame::summary() const -> std::string {
    std::ostringstream stream;
    stream << "Render frame '" << m_frameId << "' prepared " << m_drawItems.size()
           << " draw items for viewport " << m_viewportWidth << "x" << m_viewportHeight << ".";
    return stream.str();
}

auto RenderFrame::toJson() const -> nlohmann::json {
    nlohmann::json draw_items_json = nlohmann::json::array();
    for(const auto& draw_item : m_drawItems) {
        draw_items_json.push_back(draw_item.toJson());
    }

    return {{"frameId", m_frameId},
            {"sceneId", m_sceneId},
            {"viewportWidth", m_viewportWidth},
            {"viewportHeight", m_viewportHeight},
            {"camera", m_camera.toJson()},
            {"drawItemCount", m_drawItems.size()},
            {"drawItems", std::move(draw_items_json)},
            {"summary", summary()}};
}

auto buildRenderFrame(const OGL::Scene::SceneGraph& scene_graph,
                      const nlohmann::json& params) -> RenderFrame {
    const int viewport_width = std::max(params.value("viewportWidth", 1280), 64);
    const int viewport_height = std::max(params.value("viewportHeight", 720), 64);
    const std::string highlighted_node_id = params.value("highlightNodeId", std::string{});

    CameraPose camera{.yawDegrees = params.value("cameraYawDegrees", 32.0),
                      .pitchDegrees = params.value("cameraPitchDegrees", -18.0),
                      .distance = params.value("cameraDistance", 8.5)};

    std::vector<DrawItem> draw_items;
    draw_items.reserve(scene_graph.nodes().size());

    for(std::size_t index = 0; index < scene_graph.nodes().size(); ++index) {
        const auto& node = scene_graph.nodes()[index];
        const bool highlighted = !highlighted_node_id.empty() && highlighted_node_id == node.nodeId;
        draw_items.push_back({.nodeId = node.nodeId,
                              .pipelineKey = node.renderPrimitive,
                               .colorHex = K_RENDER_PALETTE[index % K_RENDER_PALETTE.size()],
                               .highlighted = highlighted});
    }

    return RenderFrame(scene_graph.sceneId() + "::frame", scene_graph.sceneId(), viewport_width,
                       viewport_height, camera, std::move(draw_items));
}

} // namespace OGL::Render
