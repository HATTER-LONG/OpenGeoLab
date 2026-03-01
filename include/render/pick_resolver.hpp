/**
 * @file pick_resolver.hpp
 * @brief GL-free pick entity resolver — priority-based selection and hierarchy lookup.
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_types.hpp"

#include <cstdint>
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
 * It references PickResolutionData directly (no copies) for efficient
 * hierarchy lookups. The PickResolutionData must outlive the PickResolver
 * while the pointer is set.
 */
class PickResolver {
public:
    /**
     * @brief Set reference to pick resolution data (no copy).
     *
     * Call when geometry data changes. The PickResolutionData must outlive
     * this PickResolver while the pointer is active.
     *
     * @param pickData  Pick-specific lookup tables from RenderData.
     */
    void setPickData(const PickResolutionData& pickData);

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

    /** @brief Clear the pick data reference. */
    void clear();

private:
    [[nodiscard]] uint64_t resolvePartUid(uint64_t uid, RenderEntityType type) const;
    [[nodiscard]] uint64_t resolveWireUid(uint64_t edgeUid, uint64_t faceUid) const;

    /// Pointer to authoritative pick resolution data (owned by RenderData)
    const PickResolutionData* m_pickData{nullptr};
};

} // namespace OpenGeoLab::Render
