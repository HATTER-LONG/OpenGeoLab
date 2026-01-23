/**
 * @file geometry_entity.hpp
 * @brief Geometry entity hierarchy with OCC integration
 *
 * Provides a type-safe entity hierarchy that wraps OpenCASCADE topological shapes:
 * - GeometryEntity: Abstract base class with common functionality
 * - VertexEntity, EdgeEntity, WireEntity, FaceEntity, ShellEntity, SolidEntity, CompoundEntity
 *
 * Each entity type provides type-specific accessors for OCC shape downcasting
 * and geometry-specific operations.
 */

#pragma once

#include "geometry_types.hpp"

#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Pnt.hxx>


#include <kangaroo/util/noncopyable.hpp>

#include <memory>
#include <vector>

namespace OpenGeoLab::Geometry {

// Forward declarations
class GeometryEntity;
class VertexEntity;
class EdgeEntity;
class WireEntity;
class FaceEntity;
class ShellEntity;
class SolidEntity;
class CompoundEntity;

// Smart pointer typedefs
using GeometryEntityPtr = std::shared_ptr<GeometryEntity>;
using GeometryEntityWeakPtr = std::weak_ptr<GeometryEntity>;
using VertexEntityPtr = std::shared_ptr<VertexEntity>;
using EdgeEntityPtr = std::shared_ptr<EdgeEntity>;
using WireEntityPtr = std::shared_ptr<WireEntity>;
using FaceEntityPtr = std::shared_ptr<FaceEntity>;
using ShellEntityPtr = std::shared_ptr<ShellEntity>;
using SolidEntityPtr = std::shared_ptr<SolidEntity>;
using CompoundEntityPtr = std::shared_ptr<CompoundEntity>;

// =============================================================================
// Entity Visitor (for type-safe operations)
// =============================================================================

/**
 * @brief Visitor interface for type-safe entity operations
 *
 * Implement this interface to perform type-specific operations
 * without explicit type checking or casting.
 */
class IEntityVisitor {
public:
    virtual ~IEntityVisitor() = default;

    virtual void visit(VertexEntity& entity) = 0;
    virtual void visit(EdgeEntity& entity) = 0;
    virtual void visit(WireEntity& entity) = 0;
    virtual void visit(FaceEntity& entity) = 0;
    virtual void visit(ShellEntity& entity) = 0;
    virtual void visit(SolidEntity& entity) = 0;
    virtual void visit(CompoundEntity& entity) = 0;
};

// =============================================================================
// GeometryEntity - Abstract Base Class (Simplified)
// =============================================================================

/**
 * @brief Abstract base class for all geometry entities
 *
 * Provides common functionality:
 * - Dual ID system (EntityId global, EntityUID type-scoped)
 * - Parent-child hierarchy management
 * - Bounding box computation
 * - Name/label management
 *
 * Subclasses provide type-specific OCC shape access and operations.
 *
 * @note Thread-safety: Read operations are thread-safe. Modifications
 *       require external synchronization.
 */
class GeometryEntity : public std::enable_shared_from_this<GeometryEntity>,
                       public Kangaroo::Util::NonCopyMoveable {
public:
    virtual ~GeometryEntity() = default;

    // -------------------------------------------------------------------------
    // Type Information
    // -------------------------------------------------------------------------

    /**
     * @brief Get the entity type
     * @return EntityType enumeration value
     */
    [[nodiscard]] virtual EntityType entityType() const = 0;

    /**
     * @brief Get the type name as string
     * @return Human-readable type name
     */
    [[nodiscard]] virtual const char* typeName() const = 0;

    /**
     * @brief Accept a visitor for type-safe operations
     * @param visitor The visitor to accept
     */
    virtual void accept(IEntityVisitor& visitor) = 0;

    // -------------------------------------------------------------------------
    // ID Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the global unique entity ID
     * @return Globally unique EntityId
     */
    [[nodiscard]] EntityId entityId() const { return m_entityId; }

    /**
     * @brief Get the type-scoped unique ID
     * @return EntityUID unique within this entity type
     */
    [[nodiscard]] EntityUID entityUID() const { return m_entityUID; }

    // -------------------------------------------------------------------------
    // Shape Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying OCC shape (generic)
     * @return Const reference to TopoDS_Shape
     */
    [[nodiscard]] virtual const TopoDS_Shape& shape() const = 0;

    /**
     * @brief Check if entity has a valid shape
     * @return true if shape is not null
     */
    [[nodiscard]] bool hasShape() const { return !shape().IsNull(); }

    // -------------------------------------------------------------------------
    // Hierarchy Management
    // -------------------------------------------------------------------------

    /**
     * @brief Get parent entity
     * @return Weak pointer to parent, or empty if root
     */
    [[nodiscard]] GeometryEntityWeakPtr parent() const { return m_parent; }

    /**
     * @brief Get direct child entities
     * @return Vector of shared pointers to children
     */
    [[nodiscard]] const std::vector<GeometryEntityPtr>& children() const { return m_children; }

    /**
     * @brief Add a child entity
     * @param child Entity to add
     */
    void addChild(const GeometryEntityPtr& child);

    /**
     * @brief Remove a child entity
     * @param child Entity to remove
     * @return true if found and removed
     */
    bool removeChild(const GeometryEntityPtr& child);

    /**
     * @brief Set parent entity
     * @param parent New parent (weak reference)
     */
    void setParent(const GeometryEntityWeakPtr& parent) { m_parent = parent; }

    /**
     * @brief Check if this is a root entity
     * @return true if has no parent
     */
    [[nodiscard]] bool isRoot() const { return m_parent.expired(); }

    /**
     * @brief Check if entity has children
     * @return true if children exist
     */
    [[nodiscard]] bool hasChildren() const { return !m_children.empty(); }

    /**
     * @brief Get direct child count
     * @return Number of children
     */
    [[nodiscard]] size_t childCount() const { return m_children.size(); }

    // -------------------------------------------------------------------------
    // Name/Label
    // -------------------------------------------------------------------------

    /**
     * @brief Get display name
     * @return User-visible name
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief Set display name
     * @param name New name
     */
    void setName(const std::string& name) { m_name = name; }

    // -------------------------------------------------------------------------
    // Bounding Box
    // -------------------------------------------------------------------------

    /**
     * @brief Get bounding box (computed on demand)
     * @return Const reference to cached bounding box
     */
    [[nodiscard]] const BoundingBox3D& boundingBox() const;

    /**
     * @brief Invalidate bounding box cache
     */
    void invalidateBoundingBox() { m_boundingBoxValid = false; }

protected:
    /**
     * @brief Protected constructor for derived classes
     * @param type Entity type for UID generation
     */
    explicit GeometryEntity(EntityType type);

    /**
     * @brief Compute bounding box from shape
     */
    void computeBoundingBox() const;

protected:
    EntityId m_entityId{INVALID_ENTITY_ID};    ///< Global unique ID
    EntityUID m_entityUID{INVALID_ENTITY_UID}; ///< Type-scoped unique ID

    mutable BoundingBox3D m_boundingBox;    ///< Cached bounding box
    mutable bool m_boundingBoxValid{false}; ///< Bounding box validity

    GeometryEntityWeakPtr m_parent;            ///< Parent reference
    std::vector<GeometryEntityPtr> m_children; ///< Child entities

    std::string m_name; ///< Display name
};

// =============================================================================
// VertexEntity
// =============================================================================

/**
 * @brief Entity representing a topological vertex (point)
 *
 * Wraps TopoDS_Vertex and provides access to the underlying 3D point.
 * Vertices are the fundamental 0D topological elements.
 */
class VertexEntity final : public GeometryEntity {
public:
    /**
     * @brief Construct from TopoDS_Vertex
     * @param vertex OCC vertex shape
     */
    explicit VertexEntity(const TopoDS_Vertex& vertex);

    [[nodiscard]] EntityType entityType() const override { return EntityType::Vertex; }
    [[nodiscard]] const char* typeName() const override { return "Vertex"; }
    void accept(IEntityVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_vertex; }

    /**
     * @brief Get the typed OCC vertex
     * @return Const reference to TopoDS_Vertex
     */
    [[nodiscard]] const TopoDS_Vertex& vertex() const { return m_vertex; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get the 3D point location
     * @return Point3D coordinates
     */
    [[nodiscard]] Point3D point() const;

    /**
     * @brief Get the OCC gp_Pnt
     * @return gp_Pnt from BRep_Tool
     */
    [[nodiscard]] gp_Pnt occPoint() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Find all edges connected to this vertex
     * @return Vector of EdgeEntity pointers from parent hierarchy
     */
    [[nodiscard]] std::vector<EdgeEntityPtr> connectedEdges() const;

    /**
     * @brief Count the number of edges connected to this vertex
     * @return Number of connected edges
     */
    [[nodiscard]] size_t edgeCount() const;

    /**
     * @brief Check if this vertex is shared by multiple edges
     * @return true if connected to more than one edge
     */
    [[nodiscard]] bool isShared() const { return edgeCount() > 1; }

    // -------------------------------------------------------------------------
    // Distance Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Calculate distance to another vertex
     * @param other Target vertex
     * @return Euclidean distance
     */
    [[nodiscard]] double distanceTo(const VertexEntity& other) const;

    /**
     * @brief Calculate distance to a point
     * @param pt Target point
     * @return Euclidean distance
     */
    [[nodiscard]] double distanceTo(const Point3D& pt) const;

    /**
     * @brief Calculate distance to an edge
     * @param edge Target edge
     * @return Minimum distance to edge
     */
    [[nodiscard]] double distanceTo(const EdgeEntity& edge) const;

private:
    TopoDS_Vertex m_vertex;
};

// =============================================================================
// EdgeEntity
// =============================================================================

/**
 * @brief Entity representing a topological edge (curve segment)
 *
 * Wraps TopoDS_Edge and provides access to curve geometry and parameters.
 * Edges are bounded by vertices at their ends.
 */
class EdgeEntity final : public GeometryEntity {
public:
    /**
     * @brief Construct from TopoDS_Edge
     * @param edge OCC edge shape
     */
    explicit EdgeEntity(const TopoDS_Edge& edge);

    [[nodiscard]] EntityType entityType() const override { return EntityType::Edge; }
    [[nodiscard]] const char* typeName() const override { return "Edge"; }
    void accept(IEntityVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_edge; }

    /**
     * @brief Get the typed OCC edge
     * @return Const reference to TopoDS_Edge
     */
    [[nodiscard]] const TopoDS_Edge& edge() const { return m_edge; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying 3D curve
     * @return Handle to Geom_Curve, may be null for degenerated edges
     */
    [[nodiscard]] Handle(Geom_Curve) curve() const;

    /**
     * @brief Get curve parameter range
     * @param first Output first parameter
     * @param last Output last parameter
     */
    void parameterRange(double& first, double& last) const;

    /**
     * @brief Evaluate point on edge at parameter
     * @param u Parameter value
     * @return Point3D at parameter
     */
    [[nodiscard]] Point3D pointAt(double u) const;

    /**
     * @brief Get tangent vector at parameter
     * @param u Parameter value
     * @return Tangent direction (normalized)
     */
    [[nodiscard]] Vector3D tangentAt(double u) const;

    /**
     * @brief Get edge length
     * @return Curve length
     */
    [[nodiscard]] double length() const;

    /**
     * @brief Check if edge is degenerated (zero length)
     * @return true if degenerated
     */
    [[nodiscard]] bool isDegenerated() const;

    /**
     * @brief Check if edge is closed (forms a loop)
     * @return true if start and end vertex are the same
     */
    [[nodiscard]] bool isClosed() const;

    /**
     * @brief Get start point of edge
     * @return Start point coordinates
     */
    [[nodiscard]] Point3D startPoint() const;

    /**
     * @brief Get end point of edge
     * @return End point coordinates
     */
    [[nodiscard]] Point3D endPoint() const;

    /**
     * @brief Get mid point of edge
     * @return Mid point coordinates
     */
    [[nodiscard]] Point3D midPoint() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get start vertex of edge
     * @return Start vertex from parent hierarchy, or nullptr
     */
    [[nodiscard]] VertexEntityPtr startVertex() const;

    /**
     * @brief Get end vertex of edge
     * @return End vertex from parent hierarchy, or nullptr
     */
    [[nodiscard]] VertexEntityPtr endVertex() const;

    /**
     * @brief Get both vertices (start and end)
     * @return Vector of vertex pointers (0-2 elements)
     */
    [[nodiscard]] std::vector<VertexEntityPtr> getVertices() const;

    /**
     * @brief Find all faces that share this edge
     * @return Vector of FaceEntity pointers
     */
    [[nodiscard]] std::vector<FaceEntityPtr> adjacentFaces() const;

    // -------------------------------------------------------------------------
    // Distance Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Calculate minimum distance to a vertex
     * @param vertex Target vertex
     * @return Minimum distance
     */
    [[nodiscard]] double distanceTo(const VertexEntity& vertex) const;

    /**
     * @brief Calculate minimum distance to a point
     * @param pt Target point
     * @return Minimum distance
     */
    [[nodiscard]] double distanceTo(const Point3D& pt) const;

    /**
     * @brief Calculate minimum distance to another edge
     * @param other Target edge
     * @return Minimum distance
     */
    [[nodiscard]] double distanceTo(const EdgeEntity& other) const;

    /**
     * @brief Find closest point on edge to a given point
     * @param pt Query point
     * @return Closest point on the edge
     */
    [[nodiscard]] Point3D closestPointTo(const Point3D& pt) const;

    /**
     * @brief Find parameter of closest point on edge to a given point
     * @param pt Query point
     * @return Parameter value at closest point
     */
    [[nodiscard]] double closestParameterTo(const Point3D& pt) const;

private:
    TopoDS_Edge m_edge;
};

// =============================================================================
// WireEntity
// =============================================================================

/**
 * @brief Entity representing a topological wire (connected edges)
 *
 * Wraps TopoDS_Wire, representing a sequence of connected edges.
 */
class WireEntity final : public GeometryEntity {
public:
    /**
     * @brief Construct from TopoDS_Wire
     * @param wire OCC wire shape
     */
    explicit WireEntity(const TopoDS_Wire& wire);

    [[nodiscard]] EntityType entityType() const override { return EntityType::Wire; }
    [[nodiscard]] const char* typeName() const override { return "Wire"; }
    void accept(IEntityVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_wire; }

    /**
     * @brief Get the typed OCC wire
     * @return Const reference to TopoDS_Wire
     */
    [[nodiscard]] const TopoDS_Wire& wire() const { return m_wire; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Check if wire is closed
     * @return true if closed loop
     */
    [[nodiscard]] bool isClosed() const;

    /**
     * @brief Get total length of wire
     * @return Sum of all edge lengths
     */
    [[nodiscard]] double length() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get ordered list of edges in wire
     * @return Vector of edge pointers in wire order
     */
    [[nodiscard]] std::vector<EdgeEntityPtr> orderedEdges() const;

    /**
     * @brief Get number of edges in wire
     * @return Edge count
     */
    [[nodiscard]] size_t edgeCount() const;

private:
    TopoDS_Wire m_wire;
};

// =============================================================================
// FaceEntity
// =============================================================================

/**
 * @brief Entity representing a topological face (bounded surface)
 *
 * Wraps TopoDS_Face and provides access to surface geometry and UV parameters.
 */
class FaceEntity final : public GeometryEntity {
public:
    /**
     * @brief Construct from TopoDS_Face
     * @param face OCC face shape
     */
    explicit FaceEntity(const TopoDS_Face& face);

    [[nodiscard]] EntityType entityType() const override { return EntityType::Face; }
    [[nodiscard]] const char* typeName() const override { return "Face"; }
    void accept(IEntityVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_face; }

    /**
     * @brief Get the typed OCC face
     * @return Const reference to TopoDS_Face
     */
    [[nodiscard]] const TopoDS_Face& face() const { return m_face; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying surface geometry
     * @return Handle to Geom_Surface
     */
    [[nodiscard]] Handle(Geom_Surface) surface() const;

    /**
     * @brief Get UV parameter bounds
     * @param uMin, uMax U parameter range
     * @param vMin, vMax V parameter range
     */
    void parameterBounds(double& u_min, double& u_max, double& v_min, double& v_max) const;

    /**
     * @brief Evaluate point on face at UV parameters
     * @param u U parameter
     * @param v V parameter
     * @return Point3D at (u,v)
     */
    [[nodiscard]] Point3D pointAt(double u, double v) const;

    /**
     * @brief Get surface normal at UV parameters
     * @param u U parameter
     * @param v V parameter
     * @return Unit normal vector
     */
    [[nodiscard]] Vector3D normalAt(double u, double v) const;

    /**
     * @brief Get face area
     * @return Surface area
     */
    [[nodiscard]] double area() const;

    /**
     * @brief Check face orientation
     * @return true if forward orientation
     */
    [[nodiscard]] bool isForward() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get outer wire (boundary) of face
     * @return Outer wire entity or nullptr
     */
    [[nodiscard]] WireEntityPtr outerWire() const;

    /**
     * @brief Get all wires (outer + holes)
     * @return Vector of wire entities
     */
    [[nodiscard]] std::vector<WireEntityPtr> allWires() const;

    /**
     * @brief Get number of holes in face
     * @return Number of inner wires
     */
    [[nodiscard]] size_t holeCount() const;

    /**
     * @brief Find adjacent faces (sharing an edge)
     * @return Vector of face entities sharing edges with this face
     */
    [[nodiscard]] std::vector<FaceEntityPtr> adjacentFaces() const;

    // -------------------------------------------------------------------------
    // Distance Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Calculate minimum distance to a point
     * @param pt Query point
     * @return Minimum distance
     */
    [[nodiscard]] double distanceTo(const Point3D& pt) const;

    /**
     * @brief Find closest point on face to a given point
     * @param pt Query point
     * @return Closest point on the face
     */
    [[nodiscard]] Point3D closestPointTo(const Point3D& pt) const;

private:
    TopoDS_Face m_face;
};

// =============================================================================
// ShellEntity
// =============================================================================

/**
 * @brief Entity representing a topological shell (connected faces)
 *
 * Wraps TopoDS_Shell, representing a set of connected faces.
 */
class ShellEntity final : public GeometryEntity {
public:
    /**
     * @brief Construct from TopoDS_Shell
     * @param shell OCC shell shape
     */
    explicit ShellEntity(const TopoDS_Shell& shell);

    [[nodiscard]] EntityType entityType() const override { return EntityType::Shell; }
    [[nodiscard]] const char* typeName() const override { return "Shell"; }
    void accept(IEntityVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_shell; }

    /**
     * @brief Get the typed OCC shell
     * @return Const reference to TopoDS_Shell
     */
    [[nodiscard]] const TopoDS_Shell& shell() const { return m_shell; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Check if shell is closed (watertight)
     * @return true if closed
     */
    [[nodiscard]] bool isClosed() const;

    /**
     * @brief Get total surface area of shell
     * @return Sum of all face areas
     */
    [[nodiscard]] double area() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get number of faces in shell
     * @return Face count
     */
    [[nodiscard]] size_t faceCount() const;

private:
    TopoDS_Shell m_shell;
};

// =============================================================================
// SolidEntity
// =============================================================================

/**
 * @brief Entity representing a topological solid (3D volume)
 *
 * Wraps TopoDS_Solid, representing a closed volume bounded by shells.
 */
class SolidEntity final : public GeometryEntity {
public:
    /**
     * @brief Construct from TopoDS_Solid
     * @param solid OCC solid shape
     */
    explicit SolidEntity(const TopoDS_Solid& solid);

    [[nodiscard]] EntityType entityType() const override { return EntityType::Solid; }
    [[nodiscard]] const char* typeName() const override { return "Solid"; }
    void accept(IEntityVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_solid; }

    /**
     * @brief Get the typed OCC solid
     * @return Const reference to TopoDS_Solid
     */
    [[nodiscard]] const TopoDS_Solid& solid() const { return m_solid; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get solid volume
     * @return Volume value
     */
    [[nodiscard]] double volume() const;

    /**
     * @brief Get surface area of solid
     * @return Total surface area
     */
    [[nodiscard]] double surfaceArea() const;

    /**
     * @brief Get center of mass
     * @return Center point
     */
    [[nodiscard]] Point3D centerOfMass() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get number of faces
     * @return Face count
     */
    [[nodiscard]] size_t faceCount() const;

    /**
     * @brief Get number of edges
     * @return Edge count
     */
    [[nodiscard]] size_t edgeCount() const;

    /**
     * @brief Get number of vertices
     * @return Vertex count
     */
    [[nodiscard]] size_t vertexCount() const;

private:
    TopoDS_Solid m_solid;
};

// =============================================================================
// CompoundEntity
// =============================================================================

/**
 * @brief Entity representing a compound (collection of shapes)
 *
 * Wraps TopoDS_Compound, containing multiple independent shapes.
 */
class CompoundEntity final : public GeometryEntity {
public:
    /**
     * @brief Construct from TopoDS_Compound
     * @param compound OCC compound shape
     */
    explicit CompoundEntity(const TopoDS_Compound& compound);

    /**
     * @brief Construct from generic TopoDS_Shape
     * @param shape Shape to wrap (must be compound or compsolid)
     */
    explicit CompoundEntity(const TopoDS_Shape& shape);

    [[nodiscard]] EntityType entityType() const override { return EntityType::Compound; }
    [[nodiscard]] const char* typeName() const override { return "Compound"; }
    void accept(IEntityVisitor& visitor) override { visitor.visit(*this); }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_compound; }

    /**
     * @brief Get the typed OCC compound
     * @return Const reference to TopoDS_Compound
     */
    [[nodiscard]] const TopoDS_Compound& compound() const { return m_compound; }

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get number of direct sub-shapes
     * @return Sub-shape count
     */
    [[nodiscard]] size_t subShapeCount() const;

private:
    TopoDS_Compound m_compound;
};

// =============================================================================
// Factory Function
// =============================================================================

/**
 * @brief Create appropriate entity subclass from generic OCC shape
 * @param shape TopoDS_Shape to wrap
 * @return Shared pointer to concrete entity type
 * @note Returns nullptr for null or unsupported shapes
 */
[[nodiscard]] GeometryEntityPtr createEntityFromShape(const TopoDS_Shape& shape);

// =============================================================================
// Type casting utilities
// =============================================================================

/**
 * @brief Safely cast entity to specific type
 * @tparam T Target entity type (e.g., FaceEntity)
 * @param entity Entity to cast
 * @return Shared pointer to T, or nullptr if wrong type
 */
template <typename T> [[nodiscard]] std::shared_ptr<T> entityAs(const GeometryEntityPtr& entity) {
    return std::dynamic_pointer_cast<T>(entity);
}

/**
 * @brief Check if entity is of specific type
 * @tparam T Entity type to check
 * @param entity Entity to test
 * @return true if entity is of type T
 */
template <typename T> [[nodiscard]] bool entityIs(const GeometryEntityPtr& entity) {
    return std::dynamic_pointer_cast<T>(entity) != nullptr;
}

} // namespace OpenGeoLab::Geometry
