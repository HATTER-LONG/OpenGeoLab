/// @file mesh_params.cpp
/// @brief JSON serialization for mesh generation parameters.

#include <opengeolab/mesh/mesh_params.hpp>

namespace OpenGeoLab::Mesh {

// ---------------------------------------------------------------------------
// SurfaceMeshParams
// ---------------------------------------------------------------------------

SurfaceMeshParams SurfaceMeshParams::fromJson(const nlohmann::json& j) {
    SurfaceMeshParams p;
    p.minSize = j.value("minSize", p.minSize);
    p.maxSize = j.value("maxSize", p.maxSize);
    p.algorithm = j.value("algorithm", p.algorithm);
    p.quadDominant = j.value("quadDominant", p.quadDominant);
    p.order = j.value("order", p.order);
    p.optimize = j.value("optimize", p.optimize);
    return p;
}

nlohmann::json SurfaceMeshParams::toJson() const {
    return {{"minSize", minSize},           {"maxSize", maxSize}, {"algorithm", algorithm},
            {"quadDominant", quadDominant}, {"order", order},     {"optimize", optimize}};
}

// ---------------------------------------------------------------------------
// VolumeMeshParams
// ---------------------------------------------------------------------------

VolumeMeshParams VolumeMeshParams::fromJson(const nlohmann::json& j) {
    VolumeMeshParams p;
    p.minSize = j.value("minSize", p.minSize);
    p.maxSize = j.value("maxSize", p.maxSize);
    p.algorithm = j.value("algorithm", p.algorithm);
    p.hexDominant = j.value("hexDominant", p.hexDominant);
    p.order = j.value("order", p.order);
    p.optimize = j.value("optimize", p.optimize);
    p.optimizeAlgorithm = j.value("optimizeAlgorithm", p.optimizeAlgorithm);
    return p;
}

nlohmann::json VolumeMeshParams::toJson() const {
    return {{"minSize", minSize},
            {"maxSize", maxSize},
            {"algorithm", algorithm},
            {"hexDominant", hexDominant},
            {"order", order},
            {"optimize", optimize},
            {"optimizeAlgorithm", optimizeAlgorithm}};
}

} // namespace OpenGeoLab::Mesh
