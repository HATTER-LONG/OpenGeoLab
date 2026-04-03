/**
 * @file mesh_store.cpp
 * @brief MeshStore implementation
 */

#include <opengeolab/mesh/mesh_store.hpp>

#include <mutex>

namespace OpenGeoLab::Mesh {

void MeshStore::setMesh(uint32_t shape_id, MeshEntry entry) {
    entry.shapeId = shape_id;

    MeshEntry stored_entry;
    {
        std::unique_lock lock(m_mutex);
        const auto it = m_entries.find(shape_id);
        if(it != m_entries.end()) {
            entry.version = it->second.version;
        }

        entry.markUpdated();
        auto [stored_it, inserted] = m_entries.insert_or_assign(shape_id, std::move(entry));
        static_cast<void>(inserted);
        stored_entry = stored_it->second;
    }

    meshAdded(shape_id, stored_entry);
}

bool MeshStore::removeMesh(uint32_t shape_id) {
    {
        std::unique_lock lock(m_mutex);
        if(m_entries.erase(shape_id) == 0) {
            return false;
        }
    }

    meshRemoved(shape_id);
    return true;
}

void MeshStore::clear() {
    {
        std::unique_lock lock(m_mutex);
        m_entries.clear();
    }

    storeCleared();
}

const MeshEntry* MeshStore::find(uint32_t shape_id) const {
    std::shared_lock lock(m_mutex);
    const auto it = m_entries.find(shape_id);
    return it != m_entries.end() ? &it->second : nullptr;
}

std::vector<uint32_t> MeshStore::allShapeIds() const {
    std::shared_lock lock(m_mutex);
    std::vector<uint32_t> ids;
    ids.reserve(m_entries.size());
    for(const auto& [id, entry] : m_entries) {
        static_cast<void>(entry);
        ids.push_back(id);
    }
    return ids;
}

std::size_t MeshStore::size() const {
    std::shared_lock lock(m_mutex);
    return m_entries.size();
}

bool MeshStore::empty() const {
    std::shared_lock lock(m_mutex);
    return m_entries.empty();
}

} // namespace OpenGeoLab::Mesh
