/**
 * @file geometry_module.hpp
 * @brief Geometry module facade for processing geometry requests.
 */

#pragma once

#include <opengeolab/geometry/geometry_export.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace OpenGeoLab::Scene {
class SceneGraph;
}

namespace OpenGeoLab::Geometry {

/** @brief Progress callback: (fraction 0.0–1.0, message). */
using ModuleProgressCallback = std::function<void(double, std::string_view)>;

/**
 * @brief Geometry module JSON command processor backed by OCC.
 *
 * Processes JSON-encoded geometry requests and returns JSON responses.
 * Internally owns a ShapeStore and coordinates with the SceneGraph.
 *
 * Supported actions:
 * - "create_box": { center: [x,y,z], size: [w,h,d] }
 * - "create_cylinder": { center: [x,y,z], radius, height }
 * - "create_sphere": { center: [x,y,z], radius }
 * - "create_torus": { center: [x,y,z], majorRadius, minorRadius }
 * - "list_shapes": {}
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule {
public:
    /**
     * @brief Construct with a reference to the scene graph.
     * @param graph Scene graph to write geometry nodes into.
     */
    explicit GeometryModule(Scene::SceneGraph& graph);

    ~GeometryModule();

    GeometryModule(const GeometryModule&) = delete;
    GeometryModule& operator=(const GeometryModule&) = delete;
    GeometryModule(GeometryModule&&) noexcept;
    GeometryModule& operator=(GeometryModule&&) noexcept;

    /**
     * @brief Process a JSON request and return a JSON response.
     * @param requestJson JSON-encoded request string.
     * @param progress Optional progress callback.
     * @return JSON-encoded response string.
     */
    [[nodiscard]] std::string process(std::string_view requestJson,
                                      const ModuleProgressCallback& progress = {});

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace OpenGeoLab::Geometry
