/**
 * @file shape_store.hpp
 * @brief Thread-safe storage for OCC shapes and their tessellated representations.
 */

#pragma once

#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <TopoDS_Shape.hxx>

#include <mutex>
#include <string>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Metadata associated with a stored shape.
 */
struct ShapeInfo {
    int id = 0;                     ///< Unique shape identifier.
    int sceneNodeId = 0;            ///< Corresponding SceneGraph node id.
    std::string label;              ///< Human-readable label.
    Scene::RenderMeshData faceMesh; ///< Tessellated face triangles.
    Scene::RenderMeshData edgeMesh; ///< Extracted edge lines.
    Scene::BoundingBox bounds;      ///< Axis-aligned bounding box.
};

/**
 * @brief Thread-safe storage for OCC shapes and their tessellated representations.
 *
 * Assigns monotonic integer IDs to each stored shape.
 * All public methods are guarded by an internal mutex.
 */
class ShapeStore {
public:
    /**
     * @brief Add a shape with its tessellated meshes.
     * @param shape OCC B-Rep shape (moved in, stored by value).
     * @param label Human-readable label.
     * @param face_mesh Tessellated face triangles.
     * @param edge_mesh Extracted edge lines.
     * @param bounds Shape bounding box.
     * @return Assigned unique shape ID (positive integer).
     */
    int addShape(TopoDS_Shape shape,
                 std::string label,
                 Scene::RenderMeshData face_mesh,
                 Scene::RenderMeshData edge_mesh,
                 Scene::BoundingBox bounds);

    /**
     * @brief Set the corresponding SceneGraph node ID after addNode().
     * @param shape_id Shape ID to update.
     * @param node_id SceneGraph node ID.
     */
    void setSceneNodeId(int shape_id, int node_id);

    /**
     * @brief Remove a shape by ID.
     * @param id Shape ID to remove.
     * @return True if found and removed, false otherwise.
     */
    bool removeShape(int id);

    /**
     * @brief Retrieve shape info by ID.
     * @param id Shape ID.
     * @return Copy of the shape info.
     * @throws std::out_of_range If not found.
     */
    [[nodiscard]] ShapeInfo getInfo(int id) const;

    /**
     * @brief Retrieve the OCC shape by ID.
     * @param id Shape ID.
     * @return Copy of the OCC shape.
     * @throws std::out_of_range If not found.
     */
    [[nodiscard]] TopoDS_Shape getShape(int id) const;

    /**
     * @brief Snapshot of all stored shape infos.
     * @return Vector of all shape infos.
     */
    [[nodiscard]] std::vector<ShapeInfo> allInfos() const;

    /**
     * @brief Number of stored shapes.
     * @return Shape count.
     */
    [[nodiscard]] int shapeCount() const;

    /** @brief Remove all shapes. */
    void clear();

private:
    struct Entry {
        ShapeInfo info;
        TopoDS_Shape shape;
    };

    mutable std::mutex mutex_;
    std::vector<Entry> entries_;
    int nextId_ = 1;
};

} // namespace OpenGeoLab::Geometry
