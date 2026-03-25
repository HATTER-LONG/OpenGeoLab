/**
 * @file shape_store.cpp
 * @brief Implementation of thread-safe OCC shape storage.
 */

#include "shape_store.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace OpenGeoLab::Geometry {

int ShapeStore::addShape(TopoDS_Shape shape,
                         std::string label,
                         Scene::RenderMeshData face_mesh,
                         Scene::RenderMeshData edge_mesh,
                         Scene::BoundingBox bounds) {
    const std::lock_guard lock(mutex_);
    const int id = nextId_++;

    ShapeInfo info{};
    info.id = id;
    info.label = std::move(label);
    info.faceMesh = std::move(face_mesh);
    info.edgeMesh = std::move(edge_mesh);
    info.bounds = bounds;

    entries_.push_back(Entry{std::move(info), std::move(shape)});
    return id;
}

void ShapeStore::setSceneNodeId(int shape_id, int node_id) {
    const std::lock_guard lock(mutex_);
    auto it = std::ranges::find_if(
        entries_, [shape_id](const Entry& entry) { return entry.info.id == shape_id; });
    if(it != entries_.end()) {
        it->info.sceneNodeId = node_id;
    }
}

bool ShapeStore::removeShape(int id) {
    const std::lock_guard lock(mutex_);
    auto it =
        std::ranges::find_if(entries_, [id](const Entry& entry) { return entry.info.id == id; });
    if(it == entries_.end()) {
        return false;
    }

    entries_.erase(it);
    return true;
}

ShapeInfo ShapeStore::getInfo(int id) const {
    const std::lock_guard lock(mutex_);
    auto it =
        std::ranges::find_if(entries_, [id](const Entry& entry) { return entry.info.id == id; });
    if(it == entries_.end()) {
        throw std::out_of_range("ShapeStore: no shape with id " + std::to_string(id));
    }

    return it->info;
}

TopoDS_Shape ShapeStore::getShape(int id) const {
    const std::lock_guard lock(mutex_);
    auto it =
        std::ranges::find_if(entries_, [id](const Entry& entry) { return entry.info.id == id; });
    if(it == entries_.end()) {
        throw std::out_of_range("ShapeStore: no shape with id " + std::to_string(id));
    }

    return it->shape;
}

std::vector<ShapeInfo> ShapeStore::allInfos() const {
    const std::lock_guard lock(mutex_);
    std::vector<ShapeInfo> result;
    result.reserve(entries_.size());
    for(const auto& entry : entries_) {
        result.push_back(entry.info);
    }
    return result;
}

int ShapeStore::shapeCount() const {
    const std::lock_guard lock(mutex_);
    return static_cast<int>(entries_.size());
}

void ShapeStore::clear() {
    const std::lock_guard lock(mutex_);
    entries_.clear();
}

} // namespace OpenGeoLab::Geometry
