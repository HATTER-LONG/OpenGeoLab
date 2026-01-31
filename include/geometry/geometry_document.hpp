#pragma once
#include "geometry/geometry_types.hpp"
#include "render/render_data.hpp"
#include "util/progress_callback.hpp"
#include <kangaroo/util/noncopyable.hpp>
#include <memory>
#include <string>

class TopoDS_Shape;
namespace OpenGeoLab::Geometry {
class GeometryDocument;
using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;
/**
 * @brief Result of a shape load operation
 */
struct LoadResult {
    bool m_success{false};                      ///< Whether the load succeeded
    std::string m_errorMessage;                 ///< Error message if failed
    EntityId m_rootEntityId{INVALID_ENTITY_ID}; ///< Root entity of loaded geometry
    size_t m_entityCount{0};                    ///< Total number of entities created

    /// Create a success result
    [[nodiscard]] static LoadResult success(EntityId root_id, size_t count) {
        LoadResult result;
        result.m_success = true;
        result.m_rootEntityId = root_id;
        result.m_entityCount = count;
        return result;
    }

    /// Create a failure result with error message
    [[nodiscard]] static LoadResult failure(const std::string& message) {
        LoadResult result;
        result.m_success = false;
        result.m_errorMessage = message;
        return result;
    }
};

class GeometryDocument : public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryDocument() = default;
    virtual ~GeometryDocument() = default;

    // -------------------------------------------------------------------------
    // Shape Loading (for io/reader)
    // -------------------------------------------------------------------------

    /**
     * @brief Load geometry from an OCC shape
     * @param shape Source OCC shape to load
     * @param name Name for the root part entity
     * @param progress Progress callback for reporting
     * @return LoadResult with success status and entity information
     *
     * @note This is the primary entry point for file readers to add
     *       geometry to the document.
     */
    [[nodiscard]] virtual LoadResult
    loadFromShape(const TopoDS_Shape& shape, // NOLINT
                  const std::string& name,
                  Util::ProgressCallback progress = Util::NO_PROGRESS_CALLBACK) = 0;

    // -------------------------------------------------------------------------
    // Render Data Generation
    // -------------------------------------------------------------------------

    /**
     * @brief Generate render data for all geometry in the document
     * @param deflection Tessellation deflection (smaller = more detailed)
     * @return RenderScene containing all renderable geometry
     *
     * @note Tessellation quality is controlled by the deflection parameter.
     *       Smaller values produce smoother surfaces but more triangles.
     */
    [[nodiscard]] virtual Render::RenderScene
    generateRenderScene(double deflection = 0.1) const = 0;

    /**
     * @brief Generate render data for a specific entity
     * @param entity_id Entity to generate render data for
     * @param deflection Tessellation deflection
     * @return RenderMesh for the specified entity
     */
    [[nodiscard]] virtual Render::RenderMesh generateRenderMesh(EntityId entity_id,
                                                                double deflection = 0.1) const = 0;
};

} // namespace OpenGeoLab::Geometry