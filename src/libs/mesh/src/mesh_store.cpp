/**
 * @file mesh_store.cpp
 * @brief MeshStore implementation with topology caching
 */

#include <opengeolab/mesh/mesh_store.hpp>

#include <mutex>

namespace OpenGeoLab::Mesh {

void MeshStore::setMesh(uint32_t shape_id, MeshEntry entry) {
    entry.shapeId = shape_id;

    MeshEntry stored_entry;
    {
        const std::unique_lock lock(m_mutex);
        const auto it = m_entries.find(shape_id);
        if(it != m_entries.end()) {
            entry.version = it->second.version;
        }

        entry.markUpdated();
        MeshTopology topology = MeshTopology::build(entry);
        auto [stored_it, inserted] = m_entries.insert_or_assign(shape_id, std::move(entry));
        static_cast<void>(inserted);
        m_topologies.insert_or_assign(shape_id, std::move(topology));
        stored_entry = stored_it->second;
    }

    meshAdded(shape_id, stored_entry);
}

void MeshStore::modifyMesh(uint32_t shape_id, std::function<void(MeshEntry&)> modifier) {
    {
        const std::unique_lock lock(m_mutex);
        const auto it = m_entries.find(shape_id);
        if(it == m_entries.end()) {
            return;
        }

        MeshEntry updated_entry = it->second;
        modifier(updated_entry);
        updated_entry.markUpdated();
        MeshTopology updated_topology = MeshTopology::build(updated_entry);

        it->second = std::move(updated_entry);
        m_topologies.insert_or_assign(shape_id, std::move(updated_topology));
    }

    meshModified(shape_id);
}

bool MeshStore::removeMesh(uint32_t shape_id) {
    {
        const std::unique_lock lock(m_mutex);
        if(m_entries.erase(shape_id) == 0) {
            return false;
        }
        m_topologies.erase(shape_id);
    }

    meshRemoved(shape_id);
    return true;
}

void MeshStore::clear() {
    {
        const std::unique_lock lock(m_mutex);
        m_entries.clear();
        m_topologies.clear();
    }

    storeCleared();
}

const MeshEntry* MeshStore::find(uint32_t shape_id) const {
    const std::shared_lock lock(m_mutex);
    const auto it = m_entries.find(shape_id);
    return it != m_entries.end() ? &it->second : nullptr;
}

const MeshTopology* MeshStore::getTopology(uint32_t shape_id) const {
    const std::shared_lock lock(m_mutex);
    const auto it = m_topologies.find(shape_id);
    return it != m_topologies.end() ? &it->second : nullptr;
}

std::vector<uint32_t> MeshStore::allShapeIds() const {
    const std::shared_lock lock(m_mutex);
    std::vector<uint32_t> ids;
    ids.reserve(m_entries.size());
    for(const auto& [id, entry] : m_entries) {
        static_cast<void>(entry);
        ids.push_back(id);
    }
    return ids;
}

std::size_t MeshStore::size() const {
    const std::shared_lock lock(m_mutex);
    return m_entries.size();
}

bool MeshStore::empty() const {
    const std::shared_lock lock(m_mutex);
    return m_entries.empty();
}

} // namespace OpenGeoLab::Mesh
