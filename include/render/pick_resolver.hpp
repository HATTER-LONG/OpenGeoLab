/**
 * @file pick_resolver.hpp
 * @brief GL-free pick entity resolver — priority-based selection and hierarchy lookup.
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_types.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Result of resolving raw GPU pick IDs to a typed entity with hierarchy context.
 */
struct ResolvedPickResult {
    uint64_t uid{0};                               ///< Entity UID
    RenderEntityType type{RenderEntityType::None}; ///< Entity type
    uint64_t partUid{0};                           ///< Parent part UID (0 = not resolvable)
    uint64_t wireUid{0};                           ///< Parent wire UID for edges (0 = N/A)
    uint64_t faceContextUid{0}; ///< Face from pick region (for wire disambiguation)

    [[nodiscard]] bool isValid() const { return uid != 0 && type != RenderEntityType::None; }
};

/**
 * @brief GL-free pick entity resolver.
 *
 * Resolves raw GPU pick IDs from the pick FBO into typed entities with
 * hierarchy context (parent Part, parent Wire). Priority-based selection
 * chooses the highest-priority entity from a region of pick IDs:
 *   Vertex > MeshNode > Edge > MeshLine > Face > Shell > Wire > Solid > Part > ...
 *
 * This class has no OpenGL dependency and can be independently tested.
 */
class PickResolver {
public:
    /**
     * @brief Rebuild entity lookup tables from geometry DrawRangeEx lists
     *        and pick resolution data.
     *
     * Call when geometry data changes (m_geometryDirty).
     *
     * @param triangles Geometry pass triangle draw ranges.
     * @param lines     Geometry pass line draw ranges.
     * @param points    Geometry pass point draw ranges.
     * @param pickData  Pick-specific lookup tables from RenderData.
     */
    void rebuild(const std::vector<DrawRangeEx>& triangles,
                 const std::vector<DrawRangeEx>& lines,
                 const std::vector<DrawRangeEx>& points,
                 const PickResolutionData& pickData);

    /**
     * @brief Resolve raw pick IDs to the highest-priority entity with hierarchy context.
     *
     * Finds the highest-priority entity from the pick region, then resolves
     * parent Part and Wire UIDs. Face context from the region is used to
     * disambiguate shared edges belonging to multiple wires.
     *
     * @param pickIds Encoded pick IDs read from the pick FBO region.
     * @return Resolved pick result with entity identity and hierarchy context.
     */
    [[nodiscard]] ResolvedPickResult resolve(const std::vector<uint64_t>& pickIds) const;

    /**
     * @brief Get all edge UIDs belonging to a wire.
     * @param wireUid Wire entity UID.
     * @return Reference to the edge UID vector (empty if not found).
     */
    [[nodiscard]] const std::vector<uint64_t>& wireEdges(uint64_t wireUid) const;

    /** @brief Clear all lookup tables. */
    void clear();

private:
    [[nodiscard]] uint64_t resolvePartUid(uint64_t uid, RenderEntityType type) const;
    [[nodiscard]] uint64_t resolveWireUid(uint64_t edgeUid, uint64_t faceUid) const;

    /// Entity uid → parent part uid (built from DrawRangeEx data)
    std::unordered_map<uint64_t, uint64_t> m_entityToPartUid;
    /// Edge uid → parent wire uid(s) (shared edges map to multiple wires)
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_edgeToWireUids;
    /// Wire uid → edge uids reverse lookup for complete wire highlighting
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_wireToEdgeUids;
    /// Wire uid → parent face uid (each wire belongs to one face)
    std::unordered_map<uint64_t, uint64_t> m_wireToFaceUid;
};

} // namespace OpenGeoLab::Render
