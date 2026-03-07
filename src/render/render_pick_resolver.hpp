/**
 * @file render_pick_resolver.hpp
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
    uint64_t m_uid{0};                               ///< Entity UID
    RenderEntityType m_type{RenderEntityType::None}; ///< Entity type
    uint64_t m_partUid{0};                           ///< Parent part UID (0 = not resolvable)
    uint64_t m_solidUid{0};                          ///< Parent solid UID for aggregate modes
    uint64_t m_wireUid{0};        ///< Wire UID (set when hovering edge in wire mode)
    uint64_t m_faceContextUid{0}; ///< Face from pick region (for wire disambiguation)

    [[nodiscard]] bool isValid() const { return m_uid != 0 && m_type != RenderEntityType::None; }
};

/**
 * @brief GL-free pick entity resolver — single point of truth for all pick/hover logic.
 *
 * Responsibilities
 * ----------------
 * 1. computeEffectiveMask() — expand the user pick-type mask so the GPU also renders
 *    sub-entities required for Wire/Solid/Part aggregate resolution (Face + Edge).
 *
 * 2. resolve() — fully resolve raw GPU pick IDs to the *final* entity including
 *    Wire/Solid/Part mode promotion and all pick constraints:
 *
 *    Sub-entity mode (Vertex / Edge / Face mix):
 *      Priority: Vertex > MeshNode > Edge > MeshLine > Face > Shell
 *      Constraints: skip already-selected, skip sub-entity of selected Part,
 *                   skip Edge whose parent Wire is selected (Add action).
 *
 *    Wire aggregate mode:
 *      Any face or face-boundary edge in the pick region resolves to the Wire that
 *      bounds the nearest face.  Face context is used to disambiguate when an edge
 *      is shared by multiple wires.
 *      Add:    skip if the wire is already selected.
 *      Remove: skip if the wire is not selected.
 *
 *    Solid aggregate mode:
 *      Any face/edge hit resolves to its parent Solid via face / edge lookup tables.
 *      Add:    skip if the solid is already selected.
 *      Remove: skip if the solid is not selected.
 *
 *    Part aggregate mode:
 *      Any face/edge hit resolves to its parent Part via m_entityToPartUid.
 *      Add:    skip if the part is already selected.
 *      Remove: skip if the part is not selected.
 *
 * This class has no OpenGL dependency.
 */
class PickResolver {
public:
    /** @brief Bind pick resolution data.  Must outlive this resolver while active. */
    void setPickData(const GeometryPickData& geometry_pick_data,
                     const MeshPickData& mesh_pick_data);

    /**
     * @brief Expand user pick-type mask for GPU rendering.
     *
     * Wire mode needs Face + Edge rendered so sub-entities can be resolved to Wire.
     * Solid / Part mode similarly need Face + Edge.
     */
    [[nodiscard]] RenderEntityTypeMask computeEffectiveMask(RenderEntityTypeMask user_mask) const;

    /**
     * @brief Fully resolve raw GPU pick IDs for the given action and user mode.
     *
     * @param pick_ids  Unique encoded pick IDs read from the pick FBO region.
     * @param action    Add or Remove.
     * @param user_mask The *user-configured* pick type mask (not the expanded GPU mask).
     * @return  Resolved entity (already promoted to Wire/Part when in those modes).
     *          isValid() == false when no actionable entity is found.
     */
    [[nodiscard]] ResolvedPickResult resolve(const std::vector<uint64_t>& pick_ids,
                                             PickAction action,
                                             RenderEntityTypeMask user_mask) const;

    /** @brief Get all edge UIDs belonging to a wire (empty if not found). */
    [[nodiscard]] const std::vector<uint64_t>& wireEdges(uint64_t wire_uid) const;

    /** @brief Clear the pick data reference. */
    void clear();

private:
    // --- Mode-specific resolvers ---
    [[nodiscard]] ResolvedPickResult resolveVEFMode(const std::vector<uint64_t>& pick_ids,
                                                    PickAction action) const;
    [[nodiscard]] ResolvedPickResult resolveWireMode(const std::vector<uint64_t>& pick_ids,
                                                     PickAction action) const;
    [[nodiscard]] ResolvedPickResult resolveSolidMode(const std::vector<uint64_t>& pick_ids,
                                                      PickAction action) const;
    [[nodiscard]] ResolvedPickResult resolvePartMode(const std::vector<uint64_t>& pick_ids,
                                                     PickAction action) const;

    // --- Hierarchy helpers ---
    [[nodiscard]] uint64_t resolvePartUid(uint64_t uid, RenderEntityType type) const;
    [[nodiscard]] uint64_t resolveWireUid(uint64_t edge_uid, uint64_t face_uid) const;
    [[nodiscard]] uint64_t resolveWireUidForFace(uint64_t face_uid) const;
    [[nodiscard]] uint64_t resolveSolidUid(uint64_t edge_uid, uint64_t face_uid) const;
    [[nodiscard]] uint64_t resolveSolidUidForFace(uint64_t face_uid) const;

    const GeometryPickData* m_geometryPickData{nullptr};
    const MeshPickData* m_meshPickData{nullptr};
};

} // namespace OpenGeoLab::Render
