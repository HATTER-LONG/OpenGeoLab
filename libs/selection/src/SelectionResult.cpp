#include <ogl/selection/SelectionResult.hpp>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace {

auto findDisplayName(const OGL::Scene::SceneGraph& scene_graph,
                     const std::string& node_id) -> std::string {
    for(const auto& node : scene_graph.nodes()) {
        if(node.nodeId == node_id) {
            return node.displayName;
        }
    }

    return node_id;
}

} // namespace

namespace OGL::Selection {

auto SelectionHit::toJson() const -> nlohmann::json {
    return {{"nodeId", nodeId},
            {"displayName", displayName},
            {"selectionType", selectionType},
            {"hitRank", hitRank}};
}

SelectionResult::SelectionResult(std::string mode,
                                 std::string frame_id,
                                 std::vector<SelectionHit> hits)
    : m_mode(std::move(mode)), m_frameId(std::move(frame_id)), m_hits(std::move(hits)) {}

auto SelectionResult::mode() const -> const std::string& { return m_mode; }

auto SelectionResult::frameId() const -> const std::string& { return m_frameId; }

auto SelectionResult::hits() const -> const std::vector<SelectionHit>& {
    return m_hits;
}

auto SelectionResult::summary() const -> std::string {
    std::ostringstream stream;
    stream << m_mode << " selection resolved " << m_hits.size() << " hit(s) from render frame '"
           << m_frameId << "'.";
    return stream.str();
}

auto SelectionResult::toJson() const -> nlohmann::json {
    nlohmann::json hits_json = nlohmann::json::array();
    for(const auto& hit : m_hits) {
        hits_json.push_back(hit.toJson());
    }

    return {{"mode", m_mode},
            {"frameId", m_frameId},
            {"hitCount", m_hits.size()},
            {"hits", std::move(hits_json)},
            {"summary", summary()}};
}

auto evaluateSelection(const OGL::Scene::SceneGraph& scene_graph,
                       const OGL::Render::RenderFrame& render_frame,
                       const nlohmann::json& params) -> SelectionResult {
    const std::string mode = params.value("mode", std::string{"pick"});
    const auto& draw_items = render_frame.drawItems();
    if(draw_items.empty()) {
        return SelectionResult(mode, render_frame.frameId(), {});
    }

    std::size_t start_index = 0;
    std::size_t selection_count = 1;

    if(mode == "box") {
        const int requested_count = params.value("selectionCount", 2);
        selection_count = static_cast<std::size_t>(
            std::clamp(requested_count, 1, static_cast<int>(draw_items.size())));
        start_index = static_cast<std::size_t>(
            std::clamp(params.value("startIndex", 0), 0, static_cast<int>(draw_items.size() - 1)));
    } else {
        const int screen_x = params.value("screenX", 0);
        const int screen_y = params.value("screenY", 0);
        start_index = static_cast<std::size_t>((std::abs(screen_x) + std::abs(screen_y)) %
                                               static_cast<int>(draw_items.size()));
    }

    std::vector<SelectionHit> hits;
    hits.reserve(selection_count);

    for(std::size_t offset = 0; offset < selection_count; ++offset) {
        const auto& draw_item = draw_items[(start_index + offset) % draw_items.size()];
        hits.push_back({.nodeId = draw_item.nodeId,
                        .displayName = findDisplayName(scene_graph, draw_item.nodeId),
                        .selectionType = mode == "box" ? "box-select" : "pick",
                        .hitRank = static_cast<int>(offset + 1)});
    }

    return SelectionResult(mode, render_frame.frameId(), std::move(hits));
}

} // namespace OGL::Selection
