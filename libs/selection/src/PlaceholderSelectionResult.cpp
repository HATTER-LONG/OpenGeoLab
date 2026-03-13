#include <ogl/selection/PlaceholderSelectionResult.hpp>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace {

auto findDisplayName(const ogl::scene::PlaceholderSceneGraph& scene_graph, const std::string& node_id)
    -> std::string {
    for(const auto& node : scene_graph.nodes()) {
        if(node.nodeId == node_id) {
            return node.displayName;
        }
    }

    return node_id;
}

} // namespace

namespace ogl::selection {

auto PlaceholderSelectionHit::toJson() const -> nlohmann::json {
    return {{"nodeId", nodeId},
            {"displayName", displayName},
            {"selectionType", selectionType},
            {"hitRank", hitRank}};
}

PlaceholderSelectionResult::PlaceholderSelectionResult(std::string mode, std::string frame_id,
                                                       std::vector<PlaceholderSelectionHit> hits)
    : m_mode(std::move(mode)), m_frameId(std::move(frame_id)), m_hits(std::move(hits)) {}

auto PlaceholderSelectionResult::mode() const -> const std::string& { return m_mode; }

auto PlaceholderSelectionResult::frameId() const -> const std::string& { return m_frameId; }

auto PlaceholderSelectionResult::hits() const -> const std::vector<PlaceholderSelectionHit>& {
    return m_hits;
}

auto PlaceholderSelectionResult::summary() const -> std::string {
    std::ostringstream stream;
    stream << "Placeholder " << m_mode << " selection resolved " << m_hits.size()
           << " hit(s) from render frame '" << m_frameId << "'.";
    return stream.str();
}

auto PlaceholderSelectionResult::toJson() const -> nlohmann::json {
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

auto evaluatePlaceholderSelection(const ogl::scene::PlaceholderSceneGraph& scene_graph,
                                  const ogl::render::PlaceholderRenderFrame& render_frame,
                                  const nlohmann::json& params) -> PlaceholderSelectionResult {
    const std::string mode = params.value("mode", std::string{"pick"});
    const auto& draw_items = render_frame.drawItems();
    if(draw_items.empty()) {
        return PlaceholderSelectionResult(mode, render_frame.frameId(), {});
    }

    std::size_t start_index = 0;
    std::size_t selection_count = 1;

    if(mode == "box") {
        const int requested_count = params.value("selectionCount", 2);
        selection_count = static_cast<std::size_t>(std::clamp(requested_count, 1,
                                                              static_cast<int>(draw_items.size())));
        start_index = static_cast<std::size_t>(
            std::clamp(params.value("startIndex", 0), 0, static_cast<int>(draw_items.size() - 1)));
    } else {
        const int screen_x = params.value("screenX", 0);
        const int screen_y = params.value("screenY", 0);
        start_index = static_cast<std::size_t>((std::abs(screen_x) + std::abs(screen_y)) %
                                               static_cast<int>(draw_items.size()));
    }

    std::vector<PlaceholderSelectionHit> hits;
    hits.reserve(selection_count);

    for(std::size_t offset = 0; offset < selection_count; ++offset) {
        const auto& draw_item = draw_items[(start_index + offset) % draw_items.size()];
        hits.push_back({.nodeId = draw_item.nodeId,
                        .displayName = findDisplayName(scene_graph, draw_item.nodeId),
                        .selectionType = mode == "box" ? "box-select" : "pick",
                        .hitRank = static_cast<int>(offset + 1)});
    }

    return PlaceholderSelectionResult(mode, render_frame.frameId(), std::move(hits));
}

} // namespace ogl::selection