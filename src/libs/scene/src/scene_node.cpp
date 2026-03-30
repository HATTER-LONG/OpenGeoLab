#include <opengeolab/scene/scene_node.hpp>

#include <utility>

namespace OpenGeoLab::Scene {

SceneNode::SceneNode(NodeId id, std::string name) : m_id(id), m_name(std::move(name)) {}

SceneNode::~SceneNode() = default;

NodeId SceneNode::id() const { return m_id; }

std::string_view SceneNode::name() const { return m_name; }

void SceneNode::setName(std::string name) { m_name = std::move(name); }

glm::mat4& SceneNode::localTransform() { return m_localTransform; }

const glm::mat4& SceneNode::localTransform() const { return m_localTransform; }

glm::mat4 SceneNode::worldMatrix() const {
    glm::mat4 world = m_localTransform;
    for(const SceneNode* node = m_parent; node != nullptr; node = node->m_parent) {
        world = node->m_localTransform * world;
    }

    return world;
}

bool SceneNode::isVisible() const { return m_visible; }

void SceneNode::setVisible(bool visible) { m_visible = visible; }

DisplayMode SceneNode::displayMode() const { return m_displayMode; }

void SceneNode::setDisplayMode(DisplayMode mode) { m_displayMode = mode; }

SceneNode* SceneNode::parent() const { return m_parent; }

std::span<const std::unique_ptr<SceneNode>> SceneNode::children() const { return m_children; }

SceneNode* SceneNode::addChild(std::unique_ptr<SceneNode> child) {
    if(child == nullptr) {
        return nullptr;
    }

    child->m_parent = this;
    m_children.push_back(std::move(child));
    return m_children.back().get();
}

std::unique_ptr<SceneNode> SceneNode::removeChild(NodeId childId) {
    for(auto it = m_children.begin(); it != m_children.end(); ++it) {
        if((*it)->id() != childId) {
            continue;
        }

        std::unique_ptr<SceneNode> child = std::move(*it);
        child->m_parent = nullptr;
        m_children.erase(it);
        return child;
    }

    return nullptr;
}

SceneNode* SceneNode::findChild(NodeId childId) const {
    for(const std::unique_ptr<SceneNode>& child : m_children) {
        if(child->id() == childId) {
            return child.get();
        }
    }

    return nullptr;
}

void SceneNode::setLocalBounds(const BoundingBox3D& bounds) { m_localBounds = bounds; }

BoundingBox3D SceneNode::localBounds() const { return m_localBounds; }

BoundingBox3D SceneNode::worldBounds() const { return m_localBounds.transformed(worldMatrix()); }

void SceneNode::setRenderComponent(std::unique_ptr<IRenderComponent> comp) {
    m_renderComponent = std::move(comp);
}

void SceneNode::setPickComponent(std::unique_ptr<IPickComponent> comp) {
    m_pickComponent = std::move(comp);
}

IRenderComponent* SceneNode::renderComponent() const { return m_renderComponent.get(); }

IPickComponent* SceneNode::pickComponent() const { return m_pickComponent.get(); }

bool SceneNode::isSelected() const { return m_selected; }

void SceneNode::setSelected(bool selected) { m_selected = selected; }

bool SceneNode::isHovered() const { return m_hovered; }

void SceneNode::setHovered(bool hovered) { m_hovered = hovered; }

uint64_t SceneNode::version() const { return m_version; }

void SceneNode::markDirty() { ++m_version; }

} // namespace OpenGeoLab::Scene
