/**
 * @file topology_utils.hpp
 * @brief Shared OCC topology extraction utilities
 *
 * Provides lightweight value types (FaceInfo, EdgeInfo, VertexInfo) and free
 * functions to extract topology summaries from OCC shapes.  Used by
 * DescribeTopologyAction, QueryEntityInfoAction, and CaptureViewportAction.
 */

#pragma once

#include <opengeolab/geometry/geometry_export.hpp>

#include <nlohmann/json_fwd.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class TopoDS_Face;
class TopoDS_Edge;
class TopoDS_Vertex;
class TopoDS_Shape;

namespace OpenGeoLab::Geometry {

struct ShapeEntry;

/// @brief Summary of a topological face.
struct OPENGEOLAB_GEOMETRY_EXPORT FaceInfo {
    uint32_t localId{0};
    std::string surfaceType;
    std::array<double, 3> center{};
    std::array<double, 3> normal{};
    std::optional<std::array<double, 3>> axis;
    std::optional<double> radius;
    double area{0.0};
};

/// @brief Summary of a topological edge.
struct OPENGEOLAB_GEOMETRY_EXPORT EdgeInfo {
    uint32_t localId{0};
    std::string curveType;
    std::array<double, 3> start{};
    std::array<double, 3> end{};
    std::optional<std::array<double, 3>> center;
    std::optional<double> radius;
    double length{0.0};
};

/// @brief Summary of a topological vertex.
struct OPENGEOLAB_GEOMETRY_EXPORT VertexInfo {
    uint32_t localId{0};
    std::array<double, 3> position{};
};

// ── Extraction ───────────────────────────────────────────────

/// @brief Extract face summary from a TopoDS_Face.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT FaceInfo extractFaceInfo(uint32_t local_id,
                                                                  const TopoDS_Face& face);

/// @brief Extract edge summary from a TopoDS_Edge.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT EdgeInfo extractEdgeInfo(uint32_t local_id,
                                                                  const TopoDS_Edge& edge);

/// @brief Extract vertex summary from a TopoDS_Vertex.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT VertexInfo extractVertexInfo(uint32_t local_id,
                                                                      const TopoDS_Vertex& vertex);

// ── JSON Serialisation ───────────────────────────────────────

/// @brief Convert FaceInfo to JSON.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT nlohmann::json toJson(const FaceInfo& info);

/// @brief Convert EdgeInfo to JSON.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT nlohmann::json toJson(const EdgeInfo& info);

/// @brief Convert VertexInfo to JSON.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT nlohmann::json toJson(const VertexInfo& info);

// ── Adjacency ────────────────────────────────────────────────

/// @brief Build edge→face adjacency map (all localIds are 1-based).
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT std::unordered_map<uint32_t, std::vector<uint32_t>>
buildEdgeToFaceAdjacency(const ShapeEntry& entry);

/// @brief Build vertex→edge adjacency map (all localIds are 1-based).
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT std::unordered_map<uint32_t, std::vector<uint32_t>>
buildVertexToEdgeAdjacency(const ShapeEntry& entry);

/// @brief Build face→edge adjacency map (all localIds are 1-based).
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT std::unordered_map<uint32_t, std::vector<uint32_t>>
buildFaceToEdgeAdjacency(const ShapeEntry& entry);

// ── Bounding Box ─────────────────────────────────────────────

/// @brief Compute AABB of an OCC sub-shape.
/// @return {min, max} arrays, or nullopt if the shape is degenerate/void.
[[nodiscard]] OPENGEOLAB_GEOMETRY_EXPORT
    std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>>
    computeSubShapeBounds(const TopoDS_Shape& sub_shape);

} // namespace OpenGeoLab::Geometry
