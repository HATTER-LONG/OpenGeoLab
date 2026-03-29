/// @file mesh_params.hpp
/// @brief Mesh generation parameter structs with JSON serialization.

#pragma once

#include <nlohmann/json.hpp>

namespace OpenGeoLab::Mesh {

/// @brief Parameters for 2D surface mesh generation.
struct SurfaceMeshParams {
    double minSize{0.1};      ///< Minimum element size
    double maxSize{10.0};     ///< Maximum element size
    int algorithm{6};         ///< Algorithm: 1=MeshAdapt, 5=Delaunay, 6=Frontal-Delaunay, 7=BAMG
    bool quadDominant{false}; ///< true → recombine into quadrilaterals
    int order{1};             ///< Element order: 1=linear, 2=quadratic
    bool optimize{true};      ///< Whether to optimize the mesh

    /// @brief Deserialize from JSON.
    static SurfaceMeshParams fromJson(const nlohmann::json& j);

    /// @brief Serialize to JSON.
    [[nodiscard]] nlohmann::json toJson() const;
};

/// @brief Parameters for 3D volume mesh generation.
struct VolumeMeshParams {
    double minSize{0.1};      ///< Minimum element size
    double maxSize{10.0};     ///< Maximum element size
    int algorithm{1};         ///< Algorithm: 1=Delaunay, 4=Frontal, 10=HXT
    bool hexDominant{false};  ///< true → recombine into hexahedra
    int order{1};             ///< Element order: 1=linear, 2=quadratic
    bool optimize{true};      ///< Whether to optimize the mesh
    int optimizeAlgorithm{0}; ///< 0=Gmsh default, 1=Netgen, 2=HighOrder

    /// @brief Deserialize from JSON.
    static VolumeMeshParams fromJson(const nlohmann::json& j);

    /// @brief Serialize to JSON.
    [[nodiscard]] nlohmann::json toJson() const;
};

} // namespace OpenGeoLab::Mesh
