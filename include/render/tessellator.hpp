/**
 * @file tessellator.hpp
 * @brief Geometry tessellation for rendering
 *
 * Converts OpenCASCADE geometry into triangulated meshes suitable
 * for OpenGL rendering.
 */

#pragma once

#include "geometry/geometry_entity.hpp"
#include "render/render_data.hpp"

#include <memory>

namespace OpenGeoLab::Render {

/**
 * @brief Tessellation quality settings
 */
struct TessellationSettings {
    double linearDeflection{0.1};  ///< Linear deflection tolerance
    double angularDeflection{0.5}; ///< Angular deflection in radians
    bool relativeDeflection{true}; ///< Use relative deflection mode
    int curveDiscretization{30};   ///< Segments per curve for edges

    /**
     * @brief Create low-quality settings (fast)
     */
    static TessellationSettings Low() { return TessellationSettings{0.5, 1.0, true, 15}; }

    /**
     * @brief Create medium-quality settings (balanced)
     */
    static TessellationSettings Medium() { return TessellationSettings{0.1, 0.5, true, 30}; }

    /**
     * @brief Create high-quality settings (detailed)
     */
    static TessellationSettings High() { return TessellationSettings{0.01, 0.1, true, 60}; }
};

/**
 * @brief Tessellates geometry entities into render meshes
 *
 * Uses OpenCASCADE's built-in tessellation to convert BRep geometry
 * into triangulated surfaces suitable for GPU rendering.
 */
class Tessellator {
public:
    Tessellator();
    explicit Tessellator(const TessellationSettings& settings);
    ~Tessellator() = default;

    /**
     * @brief Get current tessellation settings
     */
    [[nodiscard]] const TessellationSettings& settings() const { return m_settings; }

    /**
     * @brief Set tessellation settings
     */
    void setSettings(const TessellationSettings& settings) { m_settings = settings; }

    /**
     * @brief Tessellate a part entity and all its sub-entities
     * @param part The part to tessellate
     * @return Vector of render meshes for faces
     */
    std::vector<RenderMeshPtr> tessellatePart(const std::shared_ptr<Geometry::PartEntity>& part);

    /**
     * @brief Tessellate a single face entity
     * @param face The face to tessellate
     * @return Render mesh containing triangulated face data
     */
    RenderMeshPtr tessellateFace(const std::shared_ptr<Geometry::FaceEntity>& face);

    /**
     * @brief Tessellate a single edge entity
     * @param edge The edge to tessellate
     * @return Render mesh containing line segment data
     */
    RenderMeshPtr tessellateEdge(const std::shared_ptr<Geometry::EdgeEntity>& edge);

    /**
     * @brief Tessellate a solid entity (all faces and edges)
     * @param solid The solid to tessellate
     * @return Render mesh containing all face and edge data
     */
    RenderMeshPtr tessellateSolid(const std::shared_ptr<Geometry::SolidEntity>& solid);

    /**
     * @brief Tessellate a shape directly
     * @param shape The TopoDS_Shape to tessellate
     * @param entityId Entity ID to assign to vertices
     * @return Render mesh containing tessellated data
     */
    RenderMeshPtr tessellateShape(const TopoDS_Shape& shape, Geometry::EntityId entityId);

private:
    /**
     * @brief Ensure the shape has triangulation
     */
    void ensureTriangulation(const TopoDS_Shape& shape);

    /**
     * @brief Extract face triangulation data
     */
    void extractFaceData(const TopoDS_Shape& shape, RenderMesh& mesh);

    /**
     * @brief Extract edge polyline data
     */
    void extractEdgeData(const TopoDS_Shape& shape, RenderMesh& mesh);

    /**
     * @brief Calculate bounding box for mesh
     */
    void calculateBoundingBox(RenderMesh& mesh);

private:
    TessellationSettings m_settings;
};

} // namespace OpenGeoLab::Render
