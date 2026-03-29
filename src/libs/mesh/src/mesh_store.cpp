/// @file mesh_store.cpp
/// @brief MeshStore implementation — add/remove/find with mutex and signals.

#include <opengeolab/mesh/mesh_store.hpp>

#include <utility>

namespace OpenGeoLab::Mesh {

MeshStore::MeshStore() = default;
MeshStore::~MeshStore() = default;

uint32_t MeshStore::add(MeshEntry entry) {
    uint32_t id{};
    std::shared_ptr<MeshEntry> entry_ptr;

    {
        const std::lock_guard lock(m_mutex);
        id = m_nextId++;
        entry.id = id;

        entry_ptr = std::make_shared<MeshEntry>(std::move(entry));
        m_entries.push_back(entry_ptr);
    }

    // entryPtr holds a strong reference — safe even if another thread removes this ID.
    meshAdded.emit(id, *entry_ptr);
    return id;
}

void MeshStore::remove(uint32_t mesh_id) {
    {
        const std::lock_guard lock(m_mutex);
        if(mesh_id == 0 || mesh_id > m_entries.size()) {
            return;
        }
        auto& slot = m_entries[mesh_id - 1];
        if(!slot) {
            return;
        }
        slot.reset();
    }

    meshRemoved.emit(mesh_id);
}

std::shared_ptr<const MeshEntry> MeshStore::find(uint32_t mesh_id) const {
    const std::lock_guard lock(m_mutex);
    if(mesh_id == 0 || mesh_id > m_entries.size()) {
        return nullptr;
    }
    return m_entries[mesh_id - 1];
}

std::shared_ptr<MeshEntry> MeshStore::findMutable(uint32_t mesh_id) {
    const std::lock_guard lock(m_mutex);
    if(mesh_id == 0 || mesh_id > m_entries.size()) {
        return nullptr;
    }
    return m_entries[mesh_id - 1];
}

std::vector<uint32_t> MeshStore::allMeshIds() const {
    const std::lock_guard lock(m_mutex);
    std::vector<uint32_t> ids;
    for(size_t i = 0; i < m_entries.size(); ++i) {
        if(m_entries[i]) {
            ids.push_back(static_cast<uint32_t>(i + 1));
        }
    }
    return ids;
}

std::vector<uint32_t> MeshStore::findByShapeId(uint32_t shape_id) const {
    const std::lock_guard lock(m_mutex);
    std::vector<uint32_t> result;
    for(size_t i = 0; i < m_entries.size(); ++i) {
        if(m_entries[i] && m_entries[i]->sourceShapeId.has_value() &&
           m_entries[i]->sourceShapeId.value() == shape_id) {
            result.push_back(static_cast<uint32_t>(i + 1));
        }
    }
    return result;
}

} // namespace OpenGeoLab::Mesh
