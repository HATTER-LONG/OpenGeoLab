#include <opengeolab/geometry/scene_store.hpp>

#include <mutex>
#include <utility>

namespace OpenGeoLab::Geometry {

int SceneStore::addBox(BoxData box) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    const int id = m_nextId++;
    m_boxes.emplace_back(id, std::move(box));
    return id;
}

std::vector<std::pair<int, BoxData>> SceneStore::allBoxes() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_boxes;
}

std::size_t SceneStore::boxCount() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_boxes.size();
}

void SceneStore::clear() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_boxes.clear();
}

} // namespace OpenGeoLab::Geometry
