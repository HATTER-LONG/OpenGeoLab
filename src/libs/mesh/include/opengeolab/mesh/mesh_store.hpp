/**
 * @file mesh_store.hpp
 * @brief MeshStore — thread-safe per-shapeId mesh data storage
 *
 * Groups mesh data by source geometry shapeId. Provides O(1) lookup
 * via contiguous vector storage within each MeshEntry.
 */

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

#include <kangaroo/util/signal.hpp>

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Mesh {

/**
 * @brief Centralised mesh data store grouped by source geometry shapeId.
 *
 * Thread-safe: readers acquire shared lock, writers acquire exclusive lock.
 * Signals are emitted outside the lock to avoid deadlock with listeners.
 */
class OPENGEOLAB_MESH_EXPORT MeshStore {
public:
    MeshStore() = default;

    /**
     * @brief Set (replace or create) mesh data for a shape.
     * @param shape_id Source geometry shape ID
     * @param entry Mesh data to store (moved in)
     */
    void setMesh(uint32_t shape_id, MeshEntry entry);

    /**
     * @brief Remove mesh data for a shape.
     * @param shape_id Source geometry shape ID
     * @return true if data was found and removed
     */
    bool removeMesh(uint32_t shape_id);

    /// Remove all mesh data. Emits storeCleared.
    void clear();

    /**
     * @brief Find mesh data for a shape. Returns nullptr if not found.
     * @note The returned pointer is valid only while the caller holds no write lock.
     */
    [[nodiscard]] const MeshEntry* find(uint32_t shape_id) const;

    /// All shape IDs that currently have mesh data.
    [[nodiscard]] std::vector<uint32_t> allShapeIds() const;

    /// Number of shapes with mesh data.
    [[nodiscard]] std::size_t size() const;

    /// True if no mesh data is stored.
    [[nodiscard]] bool empty() const;

    /// Emitted after setMesh(). Parameters: (shapeId, entry).
    Kangaroo::Util::Signal<uint32_t, const MeshEntry&> meshAdded;

    /// Emitted after removeMesh(). Parameter: (shapeId).
    Kangaroo::Util::Signal<uint32_t> meshRemoved;

    /// Emitted after clear().
    Kangaroo::Util::Signal<> storeCleared;

private:
    std::unordered_map<uint32_t, MeshEntry> m_entries;
    mutable std::shared_mutex m_mutex;
};

} // namespace OpenGeoLab::Mesh
