/**
 * @file SelectionResult.hpp
 * @brief Selection result types used to validate selection-layer data flow.
 */

#pragma once

#include <ogl/render/RenderFrame.hpp>
#include <ogl/scene/SceneGraph.hpp>
#include <ogl/selection/export.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace OGL::Selection {

/**
 * @brief One hit returned by the selection layer.
 */
struct OGL_SELECTION_EXPORT SelectionHit {
    std::string nodeId;
    std::string displayName;
    std::string selectionType;
    int hitRank{0};

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Selection result built from render-frame and scene data.
 */
class OGL_SELECTION_EXPORT SelectionResult {
public:
    SelectionResult(std::string mode, std::string frame_id, std::vector<SelectionHit> hits);

    [[nodiscard]] auto mode() const -> const std::string&;
    [[nodiscard]] auto frameId() const -> const std::string&;
    [[nodiscard]] auto hits() const -> const std::vector<SelectionHit>&;
    [[nodiscard]] auto summary() const -> std::string;
    [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
    std::string m_mode;
    std::string m_frameId;
    std::vector<SelectionHit> m_hits;
};

/**
 * @brief Evaluate a pick or box-select request from scene and render data.
 * @param scene_graph Scene-layer graph.
 * @param render_frame Render-layer frame.
 * @param params Request parameters including mode and screen-space hints.
 * @return Selection result.
 */
OGL_SELECTION_EXPORT auto evaluateSelection(const OGL::Scene::SceneGraph& scene_graph,
                                            const OGL::Render::RenderFrame& render_frame,
                                            const nlohmann::json& params) -> SelectionResult;

} // namespace OGL::Selection
