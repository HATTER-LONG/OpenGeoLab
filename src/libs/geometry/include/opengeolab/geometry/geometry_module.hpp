/**
 * @file geometry_module.hpp
 * @brief Geometry module request processor backed by a SceneStore.
 */
#pragma once

#include <opengeolab/geometry/geometry_export.hpp>
#include <opengeolab/geometry/scene_store.hpp>

#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace OpenGeoLab::Geometry {

/** @brief Progress callback for geometry module operations. */
using ModuleProgressCallback = std::function<void(double, std::string_view)>;

/**
 * @brief Geometry module dispatcher with persistent scene state.
 *
 * The process() method is thread-safe; concurrent calls from different threads
 * are serialised by an internal mutex.
 */
class OPENGEOLAB_GEOMETRY_EXPORT GeometryModule {
public:
    /**
     * @brief Construct a geometry module using the provided scene store.
     * @param store Scene store that owns generated boxes for this module instance.
     */
    explicit GeometryModule(SceneStore& store);

    /**
     * @brief Process a JSON request for the geometry module. Thread-safe.
     * @param request_json Full JSON request envelope.
     * @param progress_callback Optional progress reporting callback.
     * @return JSON response string.
     */
    [[nodiscard]] std::string process(std::string_view request_json,
                                      const ModuleProgressCallback& progress_callback = {});

private:
    SceneStore& m_store;
    std::mutex m_processMutex;
};

} // namespace OpenGeoLab::Geometry
