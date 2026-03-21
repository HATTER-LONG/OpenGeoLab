/**
 * @file point_store.hpp
 * @brief Declares a thread-safe in-memory store for 3-D point collections.
 */

#pragma once

#include <opengeolab/geometry/bounding_box.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

#include <cstddef>
#include <mutex>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Holds a mutable collection of Point3D values with thread-safe access.
 *
 * Intended as a lightweight shared-state container so that different command
 * objects can write and read point data across requests.  All public methods
 * acquire an internal mutex before touching the stored data.
 */
class OPENGEOLAB_GEOMETRY_EXPORT PointStore {
public:
    /**
     * @brief Replaces the stored points with the given collection.
     * @param points Vector of 3-D points to store.
     */
    void setPoints(std::vector<Point3D> points);

    /**
     * @brief Returns a snapshot copy of the stored points.
     * @note The returned vector is a deep copy; mutations do not affect the store.
     */
    [[nodiscard]] auto points() const -> std::vector<Point3D>;

    /**
     * @brief Returns the number of currently stored points.
     * @return Count of stored points.
     */
    [[nodiscard]] auto size() const -> std::size_t;

    /**
     * @brief Returns true when no points are stored.
     * @return True when the store is empty.
     */
    [[nodiscard]] auto empty() const -> bool;

private:
    mutable std::mutex mutex_;
    std::vector<Point3D> points_;
};

} // namespace OpenGeoLab::Geometry
