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
#include <optional>
#include <string_view>

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

/// Parse a string name to an EntityType. Returns std::nullopt for unknown names.
constexpr std::optional<EntityType> parseEntityType(std::string_view name) noexcept {
    if(name == "GeoVertex") {
        return EntityType::GeoVertex;
    }
    if(name == "GeoEdge") {
        return EntityType::GeoEdge;
    }
    if(name == "GeoWire") {
        return EntityType::GeoWire;
    }
    if(name == "GeoFace") {
        return EntityType::GeoFace;
    }
    if(name == "GeoSolid") {
        return EntityType::GeoSolid;
    }
    if(name == "MeshNode") {
        return EntityType::MeshNode;
    }
    if(name == "MeshEdge") {
        return EntityType::MeshEdge;
    }
    if(name == "MeshElement") {
        return EntityType::MeshElement;
    }
    if(name == "SceneNode") {
        return EntityType::SceneNode;
    }
    return std::nullopt;
}

/// Return the canonical string name for an EntityType.
constexpr std::string_view entityTypeName(EntityType type) noexcept {
    switch(type) {
    case EntityType::GeoVertex:
        return "GeoVertex";
    case EntityType::GeoEdge:
        return "GeoEdge";
    case EntityType::GeoWire:
        return "GeoWire";
    case EntityType::GeoFace:
        return "GeoFace";
    case EntityType::GeoSolid:
        return "GeoSolid";
    case EntityType::MeshNode:
        return "MeshNode";
    case EntityType::MeshEdge:
        return "MeshEdge";
    case EntityType::MeshElement:
        return "MeshElement";
    case EntityType::SceneNode:
        return "SceneNode";
    default:
        return "Unknown";
    }
}

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
