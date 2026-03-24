/**
 * @file scene_store.hpp
 * @brief Thread-safe in-memory storage for generated geometry scene data.
 */
#pragma once

#include <opengeolab/geometry/box_data.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace OpenGeoLab::Geometry {

/** @brief Thread-safe store for generated boxes in the active scene. */
class OPENGEOLAB_GEOMETRY_EXPORT SceneStore {
public:
    /**
     * @brief Add a box to the scene and assign it a unique identifier.
     * @param box Box data to store.
     * @return Assigned box identifier.
     */
    [[nodiscard]] int addBox(BoxData box);

    /**
     * @brief Return a snapshot copy of all stored boxes.
     * @return Vector of box identifier and box data pairs.
     */
    [[nodiscard]] std::vector<std::pair<int, BoxData>> allBoxes() const;

    /**
     * @brief Return the number of boxes currently stored.
     * @return Stored box count.
     */
    [[nodiscard]] std::size_t boxCount() const;

    /** @brief Remove all stored boxes from the scene. */
    void clear();

private:
    mutable std::mutex m_mutex;
    std::vector<std::pair<int, BoxData>> m_boxes;
    int m_nextId = 1;
};

} // namespace OpenGeoLab::Geometry
