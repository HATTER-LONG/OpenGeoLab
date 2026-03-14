/**
 * @file PlaceholderSelectionResult.hpp
 * @brief Placeholder selection result types used to validate selection-layer data flow.
 */

#pragma once

#include <ogl/render/PlaceholderRenderFrame.hpp>
#include <ogl/scene/PlaceholderSceneGraph.hpp>
#include <ogl/selection/export.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace OGL::Selection {

/**
 * @brief One placeholder hit returned by the selection layer.
 */
struct OGL_SELECTION_EXPORT PlaceholderSelectionHit {
    std::string nodeId;
    std::string displayName;
    std::string selectionType;
    int hitRank{0};

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Placeholder selection result built from render-frame and scene data.
 */
class OGL_SELECTION_EXPORT PlaceholderSelectionResult {
public:
    PlaceholderSelectionResult(std::string mode,
                               std::string frame_id,
                               std::vector<PlaceholderSelectionHit> hits);

    [[nodiscard]] auto mode() const -> const std::string&;
    [[nodiscard]] auto frameId() const -> const std::string&;
    [[nodiscard]] auto hits() const -> const std::vector<PlaceholderSelectionHit>&;
    [[nodiscard]] auto summary() const -> std::string;
    [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
    std::string m_mode;
    std::string m_frameId;
    std::vector<PlaceholderSelectionHit> m_hits;
};

/**
 * @brief Evaluate a placeholder pick or box-select request from scene and render data.
 * @param scene_graph Scene-layer placeholder graph.
 * @param render_frame Render-layer placeholder frame.
 * @param params Request parameters including mode and screen-space hints.
 * @return Placeholder selection result.
 */
OGL_SELECTION_EXPORT auto
evaluatePlaceholderSelection(const OGL::Scene::PlaceholderSceneGraph& scene_graph,
                             const OGL::Render::PlaceholderRenderFrame& render_frame,
                             const nlohmann::json& params) -> PlaceholderSelectionResult;

} // namespace OGL::Selection