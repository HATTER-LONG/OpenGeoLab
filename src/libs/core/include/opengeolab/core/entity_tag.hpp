/**
 * @file entity_tag.hpp
 * @brief EntityType and EntityTag — unified pick/selection identifier
 *
 * EntityTag encodes the type and local index of a geometric, mesh, or scene
 * entity.  Combined with a top-level ShapeId it forms a complete pick address
 * that every module (geometry, mesh, scene, render) can interpret without
 * coupling to OCC or any other backend type.
 */

#pragma once

#include <opengeolab/core/core_export.hpp>

#include <cstdint>

namespace OpenGeoLab::Core {

/**
 * @brief Classification of selectable entities.
 *
 * Values are grouped by domain:
 *   0–9   Geometry (OCC topology)
 *   10–19 Mesh
 *   20–29 Scene graph
 */
enum class EntityType : uint8_t {
    GeoVertex = 0, ///< Topological vertex
    GeoEdge = 1,   ///< Topological edge
    GeoWire = 2,   ///< Topological wire
    GeoFace = 3,   ///< Topological face
    GeoSolid = 4,  ///< Topological solid

    MeshNode = 10,    ///< Mesh node (vertex)
    MeshEdge = 11,    ///< Mesh edge
    MeshElement = 12, ///< Mesh element (triangle, quad, etc.)

    SceneNode = 20 ///< Scene-graph node
};

/**
 * @brief Identifies one sub-entity inside a top-level shape or scene node.
 *
 * @c type   classifies the entity (vertex, edge, face, …).
 * @c localId is the 1-based index inside the owning shape's sub-shape map.
 */
struct EntityTag {
    EntityType type;
    uint32_t localId;
};

} // namespace OpenGeoLab::Core
