/**
 * @file render_data.hpp
 * @brief Render data structures for OpenGL visualization
 *
 * Defines data structures that bridge geometry entities to the rendering
 * system, supporting efficient GPU-based rendering and picking operations.
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

/**
 * @brief Vertex data for GPU rendering
 */
struct RenderVertex {
    std::array<float, 3> position; ///< XYZ position
    std::array<float, 3> normal;   ///< Surface normal
    std::array<float, 4> color;    ///< RGBA color
    uint32_t entityId{0};          ///< Entity ID for picking

    RenderVertex() = default;
    RenderVertex(float x, float y, float z, float nx, float ny, float nz)
        : position{x, y, z}, normal{nx, ny, nz}, color{0.8f, 0.8f, 0.8f, 1.0f} {}
};

/**
 * @brief Edge vertex data for wireframe rendering
 */
struct EdgeVertex {
    std::array<float, 3> position; ///< XYZ position
    std::array<float, 4> color;    ///< RGBA color
    uint32_t entityId{0};          ///< Entity ID for picking

    EdgeVertex() = default;
    EdgeVertex(float x, float y, float z) : position{x, y, z}, color{0.0f, 0.0f, 0.0f, 1.0f} {}
};

/**
 * @brief Mesh data for a single renderable entity
 *
 * Contains triangulated face data and edge data for rendering.
 */
struct RenderMesh {
    Geometry::EntityId entityId{Geometry::INVALID_ENTITY_ID}; ///< Source entity ID
    Geometry::EntityType entityType{Geometry::EntityType::None};

    std::vector<RenderVertex> vertices;   ///< Face vertices
    std::vector<uint32_t> faceIndices;    ///< Triangle indices
    std::vector<EdgeVertex> edgeVertices; ///< Edge vertices
    std::vector<uint32_t> edgeIndices;    ///< Line indices

    Geometry::BoundingBox boundingBox; ///< Mesh bounding box
    Geometry::Color baseColor;         ///< Base display color

    bool visible{true};      ///< Visibility flag
    bool selected{false};    ///< Selection state
    bool highlighted{false}; ///< Highlight state (hover)

    /**
     * @brief Check if this mesh has face data
     */
    [[nodiscard]] bool hasFaces() const { return !faceIndices.empty(); }

    /**
     * @brief Check if this mesh has edge data
     */
    [[nodiscard]] bool hasEdges() const { return !edgeIndices.empty(); }

    /**
     * @brief Get total triangle count
     */
    [[nodiscard]] size_t triangleCount() const { return faceIndices.size() / 3; }

    /**
     * @brief Get total edge segment count
     */
    [[nodiscard]] size_t edgeCount() const { return edgeIndices.size() / 2; }

    /**
     * @brief Clear all mesh data
     */
    void clear() {
        vertices.clear();
        faceIndices.clear();
        edgeVertices.clear();
        edgeIndices.clear();
    }
};

using RenderMeshPtr = std::shared_ptr<RenderMesh>;

/**
 * @brief Scene data containing all renderable meshes
 */
struct RenderScene {
    std::vector<RenderMeshPtr> meshes; ///< All meshes in the scene
    Geometry::BoundingBox boundingBox; ///< Combined bounding box

    /**
     * @brief Add a mesh to the scene
     */
    void addMesh(RenderMeshPtr mesh) {
        if(!mesh)
            return;
        meshes.push_back(mesh);
        if(mesh->boundingBox.isValid()) {
            boundingBox.expand(mesh->boundingBox);
        }
    }

    /**
     * @brief Find a mesh by entity ID
     */
    [[nodiscard]] RenderMeshPtr findMesh(Geometry::EntityId entityId) const {
        for(const auto& mesh : meshes) {
            if(mesh->entityId == entityId) {
                return mesh;
            }
        }
        return nullptr;
    }

    /**
     * @brief Remove a mesh by entity ID
     * @return true if mesh was found and removed
     */
    bool removeMesh(Geometry::EntityId entityId) {
        auto iter =
            std::remove_if(meshes.begin(), meshes.end(), [entityId](const RenderMeshPtr& mesh) {
                return mesh->entityId == entityId;
            });
        if(iter != meshes.end()) {
            meshes.erase(iter, meshes.end());
            recalculateBoundingBox();
            return true;
        }
        return false;
    }

    /**
     * @brief Clear all meshes
     */
    void clear() {
        meshes.clear();
        boundingBox = Geometry::BoundingBox();
    }

    /**
     * @brief Get total triangle count across all meshes
     */
    [[nodiscard]] size_t totalTriangles() const {
        size_t total = 0;
        for(const auto& mesh : meshes) {
            total += mesh->triangleCount();
        }
        return total;
    }

    /**
     * @brief Recalculate the combined bounding box
     */
    void recalculateBoundingBox() {
        boundingBox = Geometry::BoundingBox();
        for(const auto& mesh : meshes) {
            if(mesh->visible && mesh->boundingBox.isValid()) {
                boundingBox.expand(mesh->boundingBox);
            }
        }
    }
};

/**
 * @brief Camera state for 3D viewing
 */
struct Camera {
    std::array<float, 3> position{0.0f, 0.0f, 100.0f}; ///< Camera position
    std::array<float, 3> target{0.0f, 0.0f, 0.0f};     ///< Look-at target
    std::array<float, 3> up{0.0f, 1.0f, 0.0f};         ///< Up vector

    float fov{45.0f};         ///< Field of view in degrees
    float nearPlane{0.1f};    ///< Near clipping plane
    float farPlane{10000.0f}; ///< Far clipping plane
    float aspectRatio{1.0f};  ///< Viewport aspect ratio

    /**
     * @brief Reset camera to default view
     */
    void reset() {
        position = {0.0f, 0.0f, 100.0f};
        target = {0.0f, 0.0f, 0.0f};
        up = {0.0f, 1.0f, 0.0f};
    }

    /**
     * @brief Fit camera to view the given bounding box
     */
    void fitToBoundingBox(const Geometry::BoundingBox& bbox);
};

/**
 * @brief Light source definition
 */
struct Light {
    std::array<float, 3> position{1.0f, 1.0f, 1.0f}; ///< Light position/direction
    std::array<float, 3> color{1.0f, 1.0f, 1.0f};    ///< Light color
    float intensity{1.0f};                           ///< Light intensity
    bool directional{true};                          ///< True for directional, false for point
};

/**
 * @brief Display settings for rendering
 */
struct DisplaySettings {
    bool showFaces{true};       ///< Render solid faces
    bool showEdges{true};       ///< Render wireframe edges
    bool showVertices{false};   ///< Render vertex points
    bool smoothShading{true};   ///< Use smooth vs flat shading
    bool backfaceCulling{true}; ///< Cull backfacing triangles
    bool depthTest{true};       ///< Enable depth testing
    bool antiAliasing{true};    ///< Enable anti-aliasing

    float edgeWidth{1.0f};  ///< Line width for edges
    float vertexSize{3.0f}; ///< Point size for vertices

    Geometry::Color backgroundColor{0.2f, 0.2f, 0.25f, 1.0f}; ///< Clear color
    Geometry::Color selectedColor{1.0f, 0.5f, 0.0f, 1.0f};    ///< Selection highlight
    Geometry::Color highlightColor{0.3f, 0.6f, 1.0f, 1.0f};   ///< Hover highlight
};

/**
 * @brief Pick result from mouse interaction
 */
struct PickResult {
    bool hit{false};                                          ///< Whether something was hit
    Geometry::EntityId entityId{Geometry::INVALID_ENTITY_ID}; ///< Hit entity ID
    Geometry::EntityType entityType{Geometry::EntityType::None};
    Geometry::Point3D point; ///< 3D hit point
    float depth{0.0f};       ///< Depth value (0-1)

    /**
     * @brief Create an empty (no hit) result
     */
    static PickResult NoHit() { return PickResult(); }

    /**
     * @brief Create a hit result
     */
    static PickResult Hit(Geometry::EntityId id,
                          Geometry::EntityType type,
                          const Geometry::Point3D& hitPoint,
                          float hitDepth) {
        PickResult result;
        result.hit = true;
        result.entityId = id;
        result.entityType = type;
        result.point = hitPoint;
        result.depth = hitDepth;
        return result;
    }
};

} // namespace OpenGeoLab::Render
