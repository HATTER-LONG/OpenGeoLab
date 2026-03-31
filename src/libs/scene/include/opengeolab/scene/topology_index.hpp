/**
 * @file topology_index.hpp
 * @brief TopologyIndex — prebuilt topology relation lookup
 *
 * Maps geometric sub-entities to their topological parents:
 * edge → wire, wire → face, face → solid.
 * Used by the pick resolver to escalate GPU pick hits to higher-level
 * entities (e.g., Wire mode, Solid mode, Part mode).
 */

#pragma once

#include <opengeolab/scene/scene_export.hpp>

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {
struct ShapeEntry;
} // namespace OpenGeoLab::Geometry

namespace OpenGeoLab::Scene {

/**
 * @brief Prebuilt topology relation index for pick-mode escalation.
 *
 * For each shape, maps:
 *   edge localId → parent wire localId
 *   wire localId → parent face localId
 *   face localId → parent solid localId
 * Also provides reverse lookups (wire → edges, solid → faces).
 */
class OPENGEOLAB_SCENE_EXPORT TopologyIndex final {
public:
    /**
     * @brief Build topology relations for one shape using OCC TopExp_Explorer.
     * @param shapeId Top-level shape identifier
     * @param entry ShapeEntry containing the TopoDS_Shape and sub-shape maps
     */
    void buildForShape(uint32_t shape_id, const Geometry::ShapeEntry& entry);

    /**
     * @brief Remove all topology data for a shape.
     */
    void removeShape(uint32_t shape_id);

    // ── Forward lookups (child → parent) ──

    /** @brief Find the wire containing an edge. */
    [[nodiscard]] std::optional<uint32_t> edgeToWire(uint32_t shape_id,
                                                     uint32_t edge_local_id) const;

    /** @brief Find the face containing a wire. */
    [[nodiscard]] std::optional<uint32_t> wireToFace(uint32_t shape_id,
                                                     uint32_t wire_local_id) const;

    /** @brief Find the solid containing a face. */
    [[nodiscard]] std::optional<uint32_t> faceToSolid(uint32_t shape_id,
                                                      uint32_t face_local_id) const;

    // ── Reverse lookups (parent → children) ──

    /** @brief Get all edges belonging to a wire. */
    [[nodiscard]] std::vector<uint32_t> wireEdges(uint32_t shape_id, uint32_t wire_local_id) const;

    /** @brief Get all faces belonging to a solid. */
    [[nodiscard]] std::vector<uint32_t> solidFaces(uint32_t shape_id,
                                                   uint32_t solid_local_id) const;

private:
    /** @brief Per-shape topology relations. */
    struct ShapeRelations {
        std::unordered_map<uint32_t, uint32_t> edgeToWireMap;  /**< edge localId → wire localId */
        std::unordered_map<uint32_t, uint32_t> wireToFaceMap;  /**< wire localId → face localId */
        std::unordered_map<uint32_t, uint32_t> faceToSolidMap; /**< face localId → solid localId */
        std::unordered_map<uint32_t, std::vector<uint32_t>> wireEdgesMap;  /**< wire → [edges] */
        std::unordered_map<uint32_t, std::vector<uint32_t>> solidFacesMap; /**< solid → [faces] */
    };

    std::unordered_map<uint32_t, ShapeRelations> m_relations;
};

} // namespace OpenGeoLab::Scene
