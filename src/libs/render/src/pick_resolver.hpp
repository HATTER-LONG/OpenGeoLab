/**
 * @file pick_resolver.hpp
 * @brief Resolves raw GPU pick IDs into typed pick results
 */

#pragma once

#include <opengeolab/render/pick_mask.hpp>
#include <opengeolab/render/pick_result.hpp>
#include <opengeolab/render/render_export.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/topology_index.hpp>

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Render {

class OPENGEOLAB_RENDER_EXPORT PickResolver final {
public:
    explicit PickResolver(const Scene::TopologyIndex& topo_index);

    /**
     * @brief Resolve a list of raw pickIds to a single best result.
     *
     * VEF mode: priority Vertex > Edge > Face.
     * Wire mode: resolve Edge → Wire via TopologyIndex.
     * Solid mode: resolve Face → Solid via TopologyIndex.
     * Part mode: return shapeId from first valid hit.
     *
     * Input is assumed sorted by distance from pick center (center-first).
     */
    [[nodiscard]] PickResult resolve(const std::vector<uint64_t>& raw_pick_ids,
                                     PickMode mode) const;

    /** @brief Resolve all unique entities for box-select. */
    [[nodiscard]] std::vector<PickResult> resolveAll(const std::vector<uint64_t>& raw_pick_ids,
                                                     PickMode mode) const;

private:
    [[nodiscard]] PickResult resolveOne(uint64_t pick_id, PickMode mode) const;
    [[nodiscard]] static int typePriority(Core::EntityType type);

    const Scene::TopologyIndex& m_topoIndex;
};

} // namespace OpenGeoLab::Render
