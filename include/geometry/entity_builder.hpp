#pragma once

#include "geometry_entity.hpp"

#include <string>

class TopoDS_Shape;

namespace OpenGeoLab::Geometry {

/**
 * @brief Build a model root as a Compound with UI-level parts.
 *
 * - Always returns a CompoundEntity root.
 * - Each direct child under the root represents an independent "part".
 * - Recursively expands OCC topology and sets up parent/child pointers.
 *
 * Notes:
 * - B-Rep topology is a DAG (edges/vertices can be shared). We therefore allow
 *   entities to be referenced from multiple parents. GeometryEntity keeps a
 *   single "primary" parent while allowing multiple parents to list it as child.
 */
[[nodiscard]] GeometryEntityPtr
buildCompoundModelWithParts(const TopoDS_Shape& model_shape,
                            const std::string& part_name_prefix = "Part");

} // namespace OpenGeoLab::Geometry
