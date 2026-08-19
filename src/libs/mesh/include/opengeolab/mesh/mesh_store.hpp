/**
 * @file mesh_store.hpp
 * @brief MeshStore — thread-safe per-shapeId mesh data storage with topology caching
 *
 * Groups mesh data by source geometry shapeId. Provides O(1) lookup
 * via contiguous vector storage within each MeshEntry.
 * Topology is cached and rebuilt automatically on add/modify.
 */

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>
#include <opengeolab/mesh/mesh_topology.hpp>

#include <kangaroo/util/signal.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Mesh {

/**
 * @brief Centralised mesh data store grouped by source geometry shapeId.
 *
 * Thread-safe: readers acquire shared lock, writers acquire exclusive lock.
 * Signals are emitted outside the lock to avoid deadlock with listeners.
 * Topology is cached alongside MeshEntry and rebuilt on add/modify.
 */
class OPENGEOLAB_MESH_EXPORT MeshStore {
public:
    MeshStore() = default;

    /**
     * @brief Set (replace or create) mesh data for a shape.
     *
     * Builds and caches MeshTopology. Emits meshAdded.
     * @param shape_id Source geometry shape ID
     * @param entry Mesh data to store (moved in)
     */
    void setMesh(uint32_t shape_id, MeshEntry entry);

    /**
     * @brief Apply an in-place modification to an existing mesh.
     *
     * Acquires exclusive lock, applies modifier, bumps version,
     * rebuilds topology cache, and emits meshModified.
     * No-op if shape_id is not found.
     *
     * @param shape_id Target mesh shape ID
     * @param modifier Function that mutates the MeshEntry in-place
     */
    void modifyMesh(uint32_t shape_id, std::function<void(MeshEntry&)> modifier);

    /**
     * @brief Remove mesh data for a shape.
     *
     * Also removes cached topology. Emits meshRemoved.
     * @param shape_id Source geometry shape ID
     * @return true if data was found and removed
     */
    bool removeMesh(uint32_t shape_id);

    /// Remove all mesh data and topology. Emits storeCleared.
    void clear();

    /**
     * @brief Find mesh data for a shape. Returns nullptr if not found.
     * @note The returned pointer is valid only while the caller holds no write lock.
     */
    [[nodiscard]] const MeshEntry* find(uint32_t shape_id) const;

    /** @brief Return a thread-safe value copy suitable for work after unlocking. */
    [[nodiscard]] std::optional<MeshEntry> meshCopy(uint32_t shape_id) const;

    /**
     * @brief Get cached topology for a shape. Returns nullptr if not found.
     * @note The returned pointer is valid only while the caller holds no write lock.
     */
    [[nodiscard]] const MeshTopology* getTopology(uint32_t shape_id) const;

    /// All shape IDs that currently have mesh data.
    [[nodiscard]] std::vector<uint32_t> allShapeIds() const;

    /// Number of shapes with mesh data.
    [[nodiscard]] std::size_t size() const;

    /// True if no mesh data is stored.
    [[nodiscard]] bool empty() const;

    /// Emitted after setMesh(). Parameters: (shapeId, entry).
    Kangaroo::Util::Signal<uint32_t, const MeshEntry&> meshAdded;

    /// Emitted after modifyMesh(). Parameter: (shapeId).
    Kangaroo::Util::Signal<uint32_t> meshModified;

    /// Emitted after removeMesh(). Parameter: (shapeId).
    Kangaroo::Util::Signal<uint32_t> meshRemoved;

    /// Emitted after clear().
    Kangaroo::Util::Signal<> storeCleared;

private:
    std::unordered_map<uint32_t, MeshEntry> m_entries;
    std::unordered_map<uint32_t, MeshTopology> m_topologies;
    mutable std::shared_mutex m_mutex;
};

} // namespace OpenGeoLab::Mesh
