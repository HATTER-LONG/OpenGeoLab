/**
 * @file render_mesh_data.hpp
 * @brief Declares lightweight render mesh payloads for scene nodes.
 */
#pragma once

#include <cstdint>
#include <vector>

namespace OpenGeoLab::Scene {

/**
 * @brief Primitive topology used to render mesh indices.
 */
enum class PrimitiveType {
    Triangles, /**< Triangle list topology. */
    Lines,     /**< Line list topology. */
    Points     /**< Point list topology. */
};

/**
 * @brief CPU-side mesh buffers ready to be uploaded to a renderer.
 */
struct RenderMeshData {
    std::vector<float> positions;                      /**< Interleaved xyz vertex positions. */
    std::vector<float> normals;                        /**< Interleaved xyz vertex normals. */
    std::vector<std::uint32_t> indices;                /**< Primitive indices. */
    PrimitiveType topology = PrimitiveType::Triangles; /**< Primitive topology. */
};

} // namespace OpenGeoLab::Scene
