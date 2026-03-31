/**
 * @file scene_node.hpp
 * @brief SceneNode — composable scene graph node
 *
 * SceneNode uses a composition pattern: each node has a local transform,
 * visibility flag, display mode, and optional render/pick component slots.
 * Nodes form a tree via parent/children relationships.
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/display_mode.hpp>
#include <opengeolab/scene/pick_component.hpp>
#include <opengeolab/scene/render_component.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace OpenGeoLab::Scene {

using NodeId = uint32_t;

/**
 * @brief A composable scene-graph node.
 *
 * Owns child nodes (unique_ptr tree). Holds optional render and pick
 * components via composition. Supports transform, visibility, display
 * mode, selection, hover, and dirty tracking.
 */
class OPENGEOLAB_SCENE_EXPORT SceneNode final {
public:
    explicit SceneNode(NodeId id, std::string name = {});
    ~SceneNode();

    SceneNode(const SceneNode&) = delete;
    SceneNode& operator=(const SceneNode&) = delete;
    SceneNode(SceneNode&&) = delete;
    SceneNode& operator=(SceneNode&&) = delete;

    /** @brief Unique node identifier. */
    [[nodiscard]] NodeId id() const;

    /** @brief Human-readable node name. */
    [[nodiscard]] std::string_view name() const;

    /** @brief Set node name. */
    void setName(std::string name);

    /** @brief Mutable reference to the local transform matrix. */
    [[nodiscard]] glm::mat4& localTransform();

    /** @brief Const reference to the local transform matrix. */
    [[nodiscard]] const glm::mat4& localTransform() const;

    /**
     * @brief Compute world matrix by chaining parent transforms.
     *
     * Walks up the parent chain multiplying local transforms.
     */
    [[nodiscard]] glm::mat4 worldMatrix() const;

    /** @brief Whether this node is visible. */
    [[nodiscard]] bool isVisible() const;

    /** @brief Set visibility. */
    void setVisible(bool visible);

    /** @brief Source type tag (e.g. "geometry", "mesh"). Empty if unset. */
    [[nodiscard]] std::string_view sourceType() const;

    /** @brief Source-domain identifier (e.g. shapeId). Zero if unset. */
    [[nodiscard]] uint32_t sourceId() const;

    /**
     * @brief Set the source origin of this node.
     * @param type Domain tag, e.g. "geometry".
     * @param id Domain-specific identifier, e.g. shapeId.
     */
    void setSource(std::string type, uint32_t id);

    /** @brief Current display mode. */
    [[nodiscard]] DisplayMode displayMode() const;

    /** @brief Set display mode. */
    void setDisplayMode(DisplayMode mode);

    /** @brief Parent node (nullptr for root). */
    [[nodiscard]] SceneNode* parent() const;

    /** @brief Child nodes. */
    [[nodiscard]] std::span<const std::unique_ptr<SceneNode>> children() const;

    /**
     * @brief Add a child node. Sets parent pointer.
     * @return Raw pointer to the added child.
     */
    SceneNode* addChild(std::unique_ptr<SceneNode> child);

    /**
     * @brief Remove and return a child by id.
     * @return The removed child, or nullptr if not found.
     */
    std::unique_ptr<SceneNode> removeChild(NodeId childId);

    /**
     * @brief Find a direct child by id.
     * @return Raw pointer to the child, or nullptr if not found.
     */
    [[nodiscard]] SceneNode* findChild(NodeId childId) const;

    /** @brief Set local-space bounding box. */
    void setLocalBounds(const BoundingBox3D& bounds);

    /** @brief Get local-space bounding box. */
    [[nodiscard]] BoundingBox3D localBounds() const;

    /** @brief Get world-space bounding box (transformed by worldMatrix). */
    [[nodiscard]] BoundingBox3D worldBounds() const;

    /** @brief Attach a render component (takes ownership). */
    void setRenderComponent(std::unique_ptr<IRenderComponent> comp);

    /** @brief Attach a pick component (takes ownership). */
    void setPickComponent(std::unique_ptr<IPickComponent> comp);

    /** @brief Access render component (may be null). */
    [[nodiscard]] IRenderComponent* renderComponent() const;

    /** @brief Access pick component (may be null). */
    [[nodiscard]] IPickComponent* pickComponent() const;

    /** @brief Whether this node is selected. */
    [[nodiscard]] bool isSelected() const;

    /** @brief Set selected state. */
    void setSelected(bool selected);

    /** @brief Whether this node is hovered. */
    [[nodiscard]] bool isHovered() const;

    /** @brief Set hovered state. */
    void setHovered(bool hovered);

    /** @brief Current version counter (incremented on markDirty). */
    [[nodiscard]] uint64_t version() const;

    /** @brief Mark this node as dirty (increments version). */
    void markDirty();

private:
    NodeId m_id;
    std::string m_name;
    glm::mat4 m_localTransform{1.0F};
    bool m_visible{true};
    std::string m_sourceType;
    uint32_t m_sourceId{0};
    DisplayMode m_displayMode{DisplayMode::SolidWithEdges};
    SceneNode* m_parent{nullptr};
    std::vector<std::unique_ptr<SceneNode>> m_children;
    BoundingBox3D m_localBounds;
    std::unique_ptr<IRenderComponent> m_renderComponent;
    std::unique_ptr<IPickComponent> m_pickComponent;
    bool m_selected{false};
    bool m_hovered{false};
    uint64_t m_version{0};
};

} // namespace OpenGeoLab::Scene
