/**
 * @file wire_entity.hpp
 * @brief Wire (edge loop) geometry entity
 *
 * WireEntity wraps an OpenCASCADE TopoDS_Wire, representing a connected
 * sequence of edges forming a path or closed loop.
 */

#pragma once

#include "edge_entity.hpp"
#include <TopoDS_Wire.hxx>

namespace OpenGeoLab::Geometry {

class WireEntity;
using WireEntityPtr = std::shared_ptr<WireEntity>;

/**
 * @brief Geometry entity representing a wire (connected edge sequence)
 *
 * WireEntity represents a connected sequence of edges. A closed wire
 * can serve as the boundary of a face. Wires can be open (path) or
 * closed (loop).
 */
class WireEntity : public GeometryEntityImpl {
public:
    explicit WireEntity(const TopoDS_Wire& wire);
    ~WireEntity() override = default;

    [[nodiscard]] bool canAddChildType(EntityType child_type) const override {
        return child_type == EntityType::Edge;
    }

    [[nodiscard]] bool canAddParentType(EntityType parent_type) const override {
        return parent_type == EntityType::Face || parent_type == EntityType::Compound;
    }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_wire; }

    [[nodiscard]] bool hasShape() const override { return !m_wire.IsNull(); }
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
} // namespace OpenGeoLab::Geometry