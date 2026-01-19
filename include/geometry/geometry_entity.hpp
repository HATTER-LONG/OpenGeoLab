/**
 * @file geometry_entity.hpp
 * @brief Base classes for geometric entities in OpenGeoLab
 *
 * Provides the foundation for all geometric entities, wrapping
 * OpenCASCADE shapes with additional metadata for selection and rendering.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <TopoDS_Shape.hxx>

#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

class GeometryEntity;
using GeometryEntityPtr = std::shared_ptr<GeometryEntity>;
using GeometryEntityWeakPtr = std::weak_ptr<GeometryEntity>;

/**
 * @brief Base class for all geometric entities
 *
 * Wraps an OpenCASCADE TopoDS_Shape with additional metadata
 * for identification, selection, and visualization purposes.
 */
class GeometryEntity : public std::enable_shared_from_this<GeometryEntity> {
public:
    GeometryEntity();
    explicit GeometryEntity(const TopoDS_Shape& shape);
    virtual ~GeometryEntity() = default;

    /**
     * @brief Get the unique identifier for this entity
     */
    [[nodiscard]] EntityId id() const { return m_id; }

    /**
     * @brief Get the entity type
     */
    [[nodiscard]] virtual EntityType type() const = 0;

    /**
     * @brief Get the human-readable name of this entity
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief Set the human-readable name of this entity
     */
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief Get the underlying OpenCASCADE shape
     */
    [[nodiscard]] const TopoDS_Shape& shape() const { return m_shape; }

    /**
     * @brief Set the underlying OpenCASCADE shape
     */
    void setShape(const TopoDS_Shape& shape);

    /**
     * @brief Check if the entity has a valid shape
     */
    [[nodiscard]] bool hasValidShape() const;

    /**
     * @brief Get the bounding box of this entity
     */
    [[nodiscard]] BoundingBox boundingBox() const;

    /**
     * @brief Check if this entity is visible
     */
    [[nodiscard]] bool isVisible() const { return m_visible; }

    /**
     * @brief Set the visibility of this entity
     */
    void setVisible(bool visible) { m_visible = visible; }

    /**
     * @brief Check if this entity is currently selected
     */
    [[nodiscard]] bool isSelected() const { return m_selected; }

    /**
     * @brief Set the selection state of this entity
     */
    void setSelected(bool selected) { m_selected = selected; }

    /**
     * @brief Get the display color of this entity
     */
    [[nodiscard]] const Color& color() const { return m_color; }

    /**
     * @brief Set the display color of this entity
     */
    void setColor(const Color& color) { m_color = color; }

    /**
     * @brief Get the parent entity (if any)
     */
    [[nodiscard]] GeometryEntityWeakPtr parent() const { return m_parent; }

    /**
     * @brief Set the parent entity
     */
    void setParent(GeometryEntityWeakPtr parent) { m_parent = parent; }

    /**
     * @brief Get the child entities
     */
    [[nodiscard]] const std::vector<GeometryEntityPtr>& children() const { return m_children; }

    /**
     * @brief Add a child entity
     */
    void addChild(GeometryEntityPtr child);

    /**
     * @brief Remove a child entity
     * @return true if the child was found and removed
     */
    bool removeChild(EntityId childId);

    /**
     * @brief Find a child entity by ID
     */
    [[nodiscard]] GeometryEntityPtr findChild(EntityId childId) const;

protected:
    EntityId m_id;
    std::string m_name;
    TopoDS_Shape m_shape;
    bool m_visible{true};
    bool m_selected{false};
    Color m_color;
    GeometryEntityWeakPtr m_parent;
    std::vector<GeometryEntityPtr> m_children;
};

/**
 * @brief Vertex entity (point)
 */
class VertexEntity : public GeometryEntity {
public:
    VertexEntity() = default;
    explicit VertexEntity(const TopoDS_Shape& shape);

    [[nodiscard]] EntityType type() const override { return EntityType::Vertex; }

    /**
     * @brief Get the 3D coordinates of this vertex
     */
    [[nodiscard]] Point3D point() const;
};

/**
 * @brief Edge entity (curve)
 */
class EdgeEntity : public GeometryEntity {
public:
    EdgeEntity() = default;
    explicit EdgeEntity(const TopoDS_Shape& shape);

    [[nodiscard]] EntityType type() const override { return EntityType::Edge; }

    /**
     * @brief Get the start vertex of this edge
     */
    [[nodiscard]] Point3D startPoint() const;

    /**
     * @brief Get the end vertex of this edge
     */
    [[nodiscard]] Point3D endPoint() const;

    /**
     * @brief Get the approximate length of this edge
     */
    [[nodiscard]] double length() const;
};

/**
 * @brief Face entity (surface)
 */
class FaceEntity : public GeometryEntity {
public:
    FaceEntity() = default;
    explicit FaceEntity(const TopoDS_Shape& shape);

    [[nodiscard]] EntityType type() const override { return EntityType::Face; }

    /**
     * @brief Get the surface area of this face
     */
    [[nodiscard]] double area() const;

    /**
     * @brief Get the normal vector at a point on this face
     * @param u Parameter in U direction
     * @param v Parameter in V direction
     */
    [[nodiscard]] Vector3D normalAt(double u, double v) const;
};

/**
 * @brief Solid entity (3D body)
 */
class SolidEntity : public GeometryEntity {
public:
    SolidEntity() = default;
    explicit SolidEntity(const TopoDS_Shape& shape);

    [[nodiscard]] EntityType type() const override { return EntityType::Solid; }

    /**
     * @brief Get the volume of this solid
     */
    [[nodiscard]] double volume() const;

    /**
     * @brief Get all faces of this solid
     */
    [[nodiscard]] std::vector<std::shared_ptr<FaceEntity>> faces() const;

    /**
     * @brief Get all edges of this solid
     */
    [[nodiscard]] std::vector<std::shared_ptr<EdgeEntity>> edges() const;

    /**
     * @brief Get all vertices of this solid
     */
    [[nodiscard]] std::vector<std::shared_ptr<VertexEntity>> vertices() const;
};

/**
 * @brief Part entity - represents a complete model part
 *
 * A Part can contain multiple solids, faces, edges, and vertices.
 * It serves as the top-level container for imported geometry.
 */
class PartEntity : public GeometryEntity {
public:
    PartEntity() = default;
    explicit PartEntity(const TopoDS_Shape& shape);

    [[nodiscard]] EntityType type() const override { return EntityType::Compound; }

    /**
     * @brief Get the file path this part was loaded from (if any)
     */
    [[nodiscard]] const std::string& filePath() const { return m_filePath; }

    /**
     * @brief Set the file path this part was loaded from
     */
    void setFilePath(const std::string& path) { m_filePath = path; }

    /**
     * @brief Build the entity hierarchy from the shape
     *
     * Extracts all sub-shapes (solids, faces, edges, vertices)
     * and creates child entities for each.
     */
    void buildHierarchy();

    /**
     * @brief Get all solid entities in this part
     */
    [[nodiscard]] std::vector<std::shared_ptr<SolidEntity>> solids() const;

    /**
     * @brief Get all face entities in this part
     */
    [[nodiscard]] std::vector<std::shared_ptr<FaceEntity>> faces() const;

private:
    std::string m_filePath;
};

} // namespace OpenGeoLab::Geometry
