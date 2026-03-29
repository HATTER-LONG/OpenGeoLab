/// @file mesh_store.hpp
/// @brief MeshStore — centralized mesh data storage with signal-based change notification.

#pragma once

#include <opengeolab/mesh/mesh_entry.hpp>
#include <opengeolab/mesh/mesh_export.hpp>

#include <kangaroo/util/signal.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace OpenGeoLab::Mesh {

/// @brief Centralized storage for mesh entries.
///
/// Mesh IDs are 1-based and increment monotonically. Removed entries leave
/// null slots (no ID reuse). Thread-safe: all public methods acquire the
/// internal mutex. Signals are emitted outside the lock.
///
/// @note find() and findMutable() return shared_ptr so that callers hold
///       a valid handle even if another thread removes the entry.
class OPENGEOLAB_MESH_EXPORT MeshStore {
public:
    MeshStore();
    ~MeshStore();

    // ── Mutators ─────────────────────────────────────────────────

    /// @brief Add a mesh entry. The entry's `id` field is overwritten.
    /// @return Assigned mesh ID (1-based, monotonically increasing).
    uint32_t add(MeshEntry entry);

    /// @brief Remove a mesh by ID.
    void remove(uint32_t mesh_id);

    // ── Queries ──────────────────────────────────────────────────

    /// @brief Find a mesh by ID. Returns nullptr if not found.
    /// The returned shared_ptr keeps the entry alive independently of the store.
    [[nodiscard]] std::shared_ptr<const MeshEntry> find(uint32_t mesh_id) const;

    /// @brief Find a mutable mesh by ID. Returns nullptr if not found.
    [[nodiscard]] std::shared_ptr<MeshEntry> findMutable(uint32_t mesh_id);

    /// @brief Return all currently valid mesh IDs.
    [[nodiscard]] std::vector<uint32_t> allMeshIds() const;

    /// @brief Find all meshes associated with a given shape ID.
    [[nodiscard]] std::vector<uint32_t> findByShapeId(uint32_t shape_id) const;

    // ── Signals ──────────────────────────────────────────────────

    Kangaroo::Util::Signal<uint32_t, const MeshEntry&> meshAdded;   ///< (id, entry)
    Kangaroo::Util::Signal<uint32_t> meshRemoved;                   ///< (id)
    Kangaroo::Util::Signal<uint32_t, const MeshEntry&> meshUpdated; ///< (id, entry)

private:
    mutable std::mutex m_mutex;
    std::vector<std::shared_ptr<MeshEntry>> m_entries; ///< index = meshId - 1
    uint32_t m_nextId{1};
};

} // namespace OpenGeoLab::Mesh
