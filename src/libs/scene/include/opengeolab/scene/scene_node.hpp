/**
 * @file scene_node.hpp
 * @brief Scene node data structure holding transform, bounds, and visual data.
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>
#include <opengeolab/core/visual_data.hpp>
#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/transform.hpp>

#include <string>

namespace OpenGeoLab::Scene {

/// A single node in the scene graph.
struct OPENGEOLAB_SCENE_EXPORT SceneNode {
    std::string id;                                             ///< Unique node identifier.
    Core::EntityTag entity;                                     ///< Entity type + numeric ID.
    Transform transform;                                        ///< Local-to-world transform.
    BoundingBox bounds;                                         ///< Axis-aligned bounding box.
    Core::VisualData visual;                                    ///< Renderable mesh data.
    Core::RenderStyle style{Core::RenderStyle::SolidWithEdges}; ///< Rendering style.
    bool visible{true};                                         ///< Visibility flag.
};

} // namespace OpenGeoLab::Scene
