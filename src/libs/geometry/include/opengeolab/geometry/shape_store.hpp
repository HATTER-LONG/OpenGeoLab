/**
 * @file shape_store.hpp
 * @brief ShapeStore — centralised OCC geometry store with signal-based change notification
 *
 * All OCC shapes live here.  Scene/render modules subscribe to signals and
 * receive VisualData without ever touching OCC types.  Thread-safe for
 * concurrent add/remove/tessellate from worker threads.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/geometry/geometry_export.hpp>
#include <opengeolab/geometry/shape_entry.hpp>
#include <opengeolab/geometry/tessellator.hpp>

#include <kangaroo/util/signal.hpp>

#include <TopoDS_Shape.hxx>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Centralised OCC shape storage with sub-shape indexing and tessellation.
 *
 * Shapes are identified by a uint32_t ShapeId.  On add, sub-shape maps
 * (vertex/edge/wire/face/solid) are built automatically.  Tessellation is
 * triggered explicitly and populates the VisualData + EntityTag cache.
 *
 * Signals are emitted **outside** the internal lock so that subscribers
 * may safely call read-only ShapeStore methods from within a slot.
 */
class OPENGEOLAB_GEOMETRY_EXPORT ShapeStore {
public:
    ShapeStore();
    ~ShapeStore();

    // ── Mutators ─────────────────────────────────────────────────

    /**
     * @brief Add a top-level shape.
     * @param name  Human-readable label
     * @param shape OCC shape (deep-copied into the entry)
     * @return Allocated ShapeId
     * @post shapeAdded signal is emitted with the new entry.
     */
    uint32_t add(const std::string& name, const TopoDS_Shape& shape);

    /**
     * @brief Remove all shapes in one operation.
     * @post storeCleared signal is emitted once (not per-shape shapeRemoved).
     */
    void clear();

    /**
     * @brief Remove a shape by id.
     * @post shapeRemoved signal is emitted.
     */
    void remove(uint32_t shape_id);

    /**
     * @brief Replace the OCC shape of an existing entry, keeping its id and name.
     * @param shape_id  Target shape
     * @param new_shape Replacement OCC shape
     * @throws std::invalid_argument if shape_id is unknown
     * @post Sub-shape maps are rebuilt; tessellation cache is cleared.
     * @post shapeUpdated signal is emitted.
     */
    void replaceShape(uint32_t shape_id, const TopoDS_Shape& new_shape);

    /**
     * @brief Rename a shape.
     * @param shape_id Target shape
     * @param new_name New display name
     * @post shapeUpdated signal is emitted.
     */
    void rename(uint32_t shape_id, const std::string& new_name);

    /**
     * @brief Tessellate (or re-tessellate) a shape.
     * @param shape_id Target shape
     * @param params   Tessellation quality parameters (default: TessellationParams{})
     * @post shapeUpdated signal is emitted.
     */
    void tessellate(uint32_t shape_id, const TessellationParams& params = {});

    // ── Queries ──────────────────────────────────────────────────

    /** @brief Look up a shape entry.  Returns nullptr if not found. */
    [[nodiscard]] const ShapeEntry* find(uint32_t shape_id) const;

    /** @brief Return all active shape ids. */
    [[nodiscard]] std::vector<uint32_t> allShapeIds() const;

    /** @brief Number of shapes currently stored. */
    [[nodiscard]] std::size_t size() const;

    /**
     * @brief Retrieve an OCC sub-shape by type and local index.
     * @param shape_id  Top-level shape id
     * @param type      Sub-shape classification
     * @param local_id  1-based index into the corresponding sub-shape map
     * @return The OCC sub-shape, or a null shape if not found
     */
    [[nodiscard]] TopoDS_Shape
    subShape(uint32_t shape_id, Core::EntityType type, uint32_t local_id) const;

    // ── Signals ──────────────────────────────────────────────────

    Kangaroo::Util::Signal<uint32_t, const ShapeEntry&> shapeAdded;   ///< (id, entry)
    Kangaroo::Util::Signal<uint32_t> shapeRemoved;                    ///< (id)
    Kangaroo::Util::Signal<uint32_t, const ShapeEntry&> shapeUpdated; ///< (id, entry)
    Kangaroo::Util::Signal<> storeCleared;                            ///< all shapes removed

private:
    void buildSubShapeIndex(ShapeEntry& entry);

    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<ShapeEntry>> m_slots;
    std::vector<uint32_t> m_freeList;
    uint32_t m_nextId{0};
};

} // namespace OpenGeoLab::Geometry
