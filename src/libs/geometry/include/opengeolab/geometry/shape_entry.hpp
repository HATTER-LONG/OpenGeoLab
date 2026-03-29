/**
 * @file shape_entry.hpp
 * @brief ShapeEntry — complete data record for one top-level OCC shape
 *
 * Each imported or created shape is stored as a ShapeEntry inside ShapeStore.
 * The entry caches indexed sub-shape maps (vertex, edge, wire, face, solid)
 * for O(1) pick-back look-up, and optionally holds tessellated VisualData
 * with per-primitive EntityTag vectors for the render pipeline.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/visual_data.hpp>

#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Shape.hxx>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Data record for one top-level OCC shape.
 *
 * Sub-shape maps are built once via TopExp::MapShapes on add/import.
 * VisualData and tag vectors are populated lazily by tessellation.
 */
struct ShapeEntry {
    uint32_t id{0};   /**< Unique ShapeId allocated by ShapeStore */
    std::string name; /**< User-given or file-derived name */

    TopoDS_Shape shape; /**< The OCC topological shape */

    /** @name Sub-shape index maps (1-based, built on add) */
    /** @{ */
    TopTools_IndexedMapOfShape vertexMap;
    TopTools_IndexedMapOfShape edgeMap;
    TopTools_IndexedMapOfShape wireMap;
    TopTools_IndexedMapOfShape faceMap;
    TopTools_IndexedMapOfShape solidMap;
    /** @} */

    /** @name Tessellation cache (populated by ShapeStore::tessellate) */
    /** @{ */
    std::shared_ptr<Core::VisualData> visualData;
    std::vector<Core::EntityTag> triangleTags; /**< One tag per triangle */
    std::vector<Core::EntityTag> edgeTags;     /**< One tag per edge segment */
    std::vector<Core::EntityTag> vertexTags;   /**< One tag per point */
    /** @} */
};

} // namespace OpenGeoLab::Geometry
