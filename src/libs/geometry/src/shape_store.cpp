/**
 * @file shape_store.cpp
 * @brief ShapeStore implementation — add/remove/tessellate with sub-shape indexing
 */

#include <opengeolab/geometry/shape_store.hpp>

#include <opengeolab/core/logger.hpp>
#include <opengeolab/geometry/tessellator.hpp>

#include <TopExp.hxx>

#include <stdexcept>
#include <utility>

namespace OpenGeoLab::Geometry {

ShapeStore::ShapeStore() = default;
ShapeStore::~ShapeStore() = default;

// ── Mutators ────────────────────────────────────────────────────

uint32_t ShapeStore::add(const std::string& name, const TopoDS_Shape& shape) {
    uint32_t id{};
    const ShapeEntry* entry_ptr{};

    {
        const std::lock_guard lock(m_mutex);

        if(!m_freeList.empty()) {
            id = m_freeList.back();
            m_freeList.pop_back();
        } else {
            id = m_nextId++;
            m_slots.emplace_back();
        }

        auto entry = std::make_unique<ShapeEntry>();
        entry->id = id;
        entry->name = name;
        entry->shape = shape;
        buildSubShapeIndex(*entry);

        m_slots[id] = std::move(entry);
        entry_ptr = m_slots[id].get();
    }

    // Emit outside lock
    shapeAdded.emit(id, *entry_ptr);
    return id;
}

void ShapeStore::rename(uint32_t shape_id, const std::string& new_name) {
    const ShapeEntry* entry_ptr{};
    {
        const std::lock_guard lock(m_mutex);
        if(shape_id >= m_slots.size() || !m_slots[shape_id]) {
            LOG_WARN("ShapeStore::rename: invalid shapeId {}", shape_id);
            return;
        }
        m_slots[shape_id]->name = new_name;
        entry_ptr = m_slots[shape_id].get();
    }

    shapeUpdated.emit(shape_id, *entry_ptr);
}

void ShapeStore::clear() {
    {
        const std::lock_guard lock(m_mutex);
        m_slots.clear();
        m_freeList.clear();
        m_nextId = 0;
    }
    storeCleared.emit();
}

void ShapeStore::remove(uint32_t shape_id) {
    {
        const std::lock_guard lock(m_mutex);
        if(shape_id >= m_slots.size() || !m_slots[shape_id]) {
            LOG_WARN("ShapeStore::remove: invalid shapeId {}", shape_id);
            return;
        }
        m_slots[shape_id].reset();
        m_freeList.push_back(shape_id);
    }

    shapeRemoved.emit(shape_id);
}

void ShapeStore::replaceShape(uint32_t shape_id, const TopoDS_Shape& new_shape) {
    const ShapeEntry* entry_ptr{};
    {
        const std::lock_guard lock(m_mutex);
        if(shape_id >= m_slots.size() || !m_slots[shape_id]) {
            throw std::invalid_argument("ShapeStore::replaceShape: unknown shapeId");
        }
        auto& entry = *m_slots[shape_id];
        entry.shape = new_shape;

        entry.vertexMap.Clear();
        entry.edgeMap.Clear();
        entry.wireMap.Clear();
        entry.faceMap.Clear();
        entry.solidMap.Clear();

        entry.visualData.reset();
        entry.triangleTags.clear();
        entry.edgeTags.clear();
        entry.vertexTags.clear();

        buildSubShapeIndex(entry);
        entry_ptr = &entry;
    }

    shapeUpdated.emit(shape_id, *entry_ptr);
}

void ShapeStore::tessellate(uint32_t shape_id, const TessellationParams& params) {
    const ShapeEntry* entry_ptr{};
    {
        const std::lock_guard lock(m_mutex);
        if(shape_id >= m_slots.size() || !m_slots[shape_id]) {
            throw std::invalid_argument("ShapeStore::tessellate: unknown shapeId");
        }
        auto& entry = *m_slots[shape_id];
        auto result = OpenGeoLab::Geometry::tessellate(entry, params);
        entry.visualData = std::make_shared<Core::VisualData>(std::move(result.visualData));
        entry.triangleTags = std::move(result.triangleTags);
        entry.edgeTags = std::move(result.edgeTags);
        entry.vertexTags = std::move(result.vertexTags);
        entry_ptr = &entry;
    }

    shapeUpdated.emit(shape_id, *entry_ptr);
}

// ── Queries ─────────────────────────────────────────────────────

const ShapeEntry* ShapeStore::find(uint32_t shape_id) const {
    const std::lock_guard lock(m_mutex);
    if(shape_id >= m_slots.size()) {
        return nullptr;
    }
    return m_slots[shape_id].get();
}

std::vector<uint32_t> ShapeStore::allShapeIds() const {
    const std::lock_guard lock(m_mutex);
    std::vector<uint32_t> ids;
    for(uint32_t i = 0; i < m_slots.size(); ++i) {
        if(m_slots[i]) {
            ids.push_back(i);
        }
    }
    return ids;
}

std::size_t ShapeStore::size() const {
    const std::lock_guard lock(m_mutex);
    std::size_t count = 0;
    for(const auto& slot : m_slots) {
        if(slot) {
            ++count;
        }
    }
    return count;
}

TopoDS_Shape
ShapeStore::subShape(uint32_t shape_id, Core::EntityType type, uint32_t local_id) const {
    const std::lock_guard lock(m_mutex);
    if(shape_id >= m_slots.size() || !m_slots[shape_id]) {
        return {};
    }
    const auto& entry = *m_slots[shape_id];

    auto safe_find = [&](const TopTools_IndexedMapOfShape& map) -> TopoDS_Shape {
        if(local_id < 1 || static_cast<int>(local_id) > map.Extent()) {
            return {};
        }
        return map.FindKey(static_cast<int>(local_id));
    };

    switch(type) {
    case Core::EntityType::GeoVertex:
        return safe_find(entry.vertexMap);
    case Core::EntityType::GeoEdge:
        return safe_find(entry.edgeMap);
    case Core::EntityType::GeoWire:
        return safe_find(entry.wireMap);
    case Core::EntityType::GeoFace:
        return safe_find(entry.faceMap);
    case Core::EntityType::GeoSolid:
        return safe_find(entry.solidMap);
    default:
        return {};
    }
}

// ── Private ─────────────────────────────────────────────────────

void ShapeStore::buildSubShapeIndex(ShapeEntry& entry) {
    TopExp::MapShapes(entry.shape, TopAbs_VERTEX, entry.vertexMap);
    TopExp::MapShapes(entry.shape, TopAbs_EDGE, entry.edgeMap);
    TopExp::MapShapes(entry.shape, TopAbs_WIRE, entry.wireMap);
    TopExp::MapShapes(entry.shape, TopAbs_FACE, entry.faceMap);
    TopExp::MapShapes(entry.shape, TopAbs_SOLID, entry.solidMap);
}

} // namespace OpenGeoLab::Geometry
