#include <opengeolab/scene/scene_graph.hpp>

#include <algorithm>
#include <utility>

namespace OpenGeoLab::Scene {

// ── Write operations ────────────────────────────────────────────────────────

void SceneGraph::addNode(SceneNode node) {
    std::unique_lock lock(m_mutex);
    const std::string id = node.id;
    m_nodes.insert_or_assign(id, std::move(node));
    m_pendingChanges.added.push_back(id);
}

void SceneGraph::updateVisual(const std::string& id, Core::VisualData visual) {
    std::unique_lock lock(m_mutex);
    auto it = m_nodes.find(id);
    if(it == m_nodes.end()) {
        return;
    }

    auto& node = it->second;
    node.visual = std::move(visual);

    // Recompute bounds from all surface positions.
    BoundingBox newBounds;
    for(const auto& surf : node.visual.surfaces) {
        if(!surf.positions.empty()) {
            newBounds.expand(BoundingBox::fromPositions(
                surf.positions.data(), surf.positions.size() / 3, sizeof(float) * 3));
        }
    }
    node.bounds = newBounds;

    m_pendingChanges.updated.push_back(id);
}

void SceneGraph::removeNode(const std::string& id) {
    std::unique_lock lock(m_mutex);
    if(m_nodes.erase(id) > 0) {
        m_pendingChanges.removed.push_back(id);
    }
}

void SceneGraph::setNodeVisibility(const std::string& id, bool visible) {
    std::unique_lock lock(m_mutex);
    auto it = m_nodes.find(id);
    if(it == m_nodes.end() || it->second.visible == visible) {
        return;
    }
    it->second.visible = visible;
    m_pendingChanges.updated.push_back(id);
}

// ── Read operations ─────────────────────────────────────────────────────────

const SceneNode* SceneGraph::findNode(const std::string& id) const {
    std::shared_lock lock(m_mutex);
    auto it = m_nodes.find(id);
    return (it != m_nodes.end()) ? &it->second : nullptr;
}

std::vector<std::string> SceneGraph::allNodeIds() const {
    std::shared_lock lock(m_mutex);
    std::vector<std::string> ids;
    ids.reserve(m_nodes.size());
    for(const auto& [key, _] : m_nodes) {
        ids.push_back(key);
    }
    return ids;
}

BoundingBox SceneGraph::sceneBounds() const {
    std::shared_lock lock(m_mutex);
    BoundingBox merged;
    for(const auto& [_, node] : m_nodes) {
        if(node.visible) {
            merged.expand(node.bounds);
        }
    }
    return merged;
}

bool SceneGraph::hasChanges() const {
    std::shared_lock lock(m_mutex);
    return !m_pendingChanges.empty();
}

// ── Changeset consumption ───────────────────────────────────────────────────

SceneGraph::Changeset SceneGraph::consumeChangeset() {
    std::unique_lock lock(m_mutex);
    return std::exchange(m_pendingChanges, Changeset{});
}

} // namespace OpenGeoLab::Scene
