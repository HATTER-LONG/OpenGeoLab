/**
 * @file shape_builder.hpp
 * @brief Build geometry entity hierarchy from OCC shapes
 *
 * ShapeBuilder traverses an OpenCASCADE TopoDS_Shape and creates a complete
 * hierarchy of GeometryEntity objects with proper parent-child relationships.
 * It supports creating PartEntity as the root with all sub-shapes.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "geometry/mesh_metadata.hpp"
#include "geometry/part_entity.hpp"
#include "geometry/render_data.hpp"

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <functional>
#include <string>

namespace OpenGeoLab::Geometry {

/**
 * @brief Progress callback for shape building operations
 * @param progress Current progress [0, 1]
 * @param message Status message
 * @return false to cancel, true to continue
 */
using BuildProgressCallback = std::function<bool(double progress, const std::string& message)>;

/**
 * @brief Configuration options for ShapeBuilder
 */
struct ShapeBuildOptions {
    bool m_buildVertices{true};  ///< Include vertex entities
    bool m_buildEdges{true};     ///< Include edge entities
    bool m_buildWires{true};     ///< Include wire entities
    bool m_buildFaces{true};     ///< Include face entities
    bool m_buildShells{true};    ///< Include shell entities
    bool m_buildSolids{true};    ///< Include solid entities
    bool m_buildCompounds{true}; ///< Include compound entities

    bool m_generateRenderData{false};   ///< Generate render data during build
    bool m_generateMeshMetadata{false}; ///< Generate mesh metadata during build

    TessellationParams m_tessellation; ///< Tessellation parameters for render data

    /// Default constructor with all options enabled
    ShapeBuildOptions() = default;

    /// Create options for render-ready building
    [[nodiscard]] static ShapeBuildOptions forRendering() {
        ShapeBuildOptions options;
        options.m_generateRenderData = true;
        options.m_tessellation = TessellationParams::mediumQuality();
        return options;
    }

    /// Create options for mesh generation preparation
    [[nodiscard]] static ShapeBuildOptions forMeshing() {
        ShapeBuildOptions options;
        options.m_generateMeshMetadata = true;
        return options;
    }

    /// Create minimal options (topology only)
    [[nodiscard]] static ShapeBuildOptions minimal() {
        ShapeBuildOptions options;
        options.m_buildVertices = false;
        options.m_buildWires = false;
        return options;
    }
};

/**
 * @brief Result of a shape building operation
 */
struct ShapeBuildResult {
    bool m_success{false};              ///< Build succeeded
    std::string m_errorMessage;         ///< Error message if failed
    PartEntityPtr m_rootPart;           ///< Root part entity
    PartRenderDataPtr m_renderData;     ///< Generated render data (if requested)
    PartMeshMetadataPtr m_meshMetadata; ///< Generated mesh metadata (if requested)

    /// Entity counts for statistics
    size_t m_vertexCount{0};
    size_t m_edgeCount{0};
    size_t m_wireCount{0};
    size_t m_faceCount{0};
    size_t m_shellCount{0};
    size_t m_solidCount{0};
    size_t m_compoundCount{0};

    /**
     * @brief Create a success result
     * @param root_part The created root part entity
     * @return Success result
     */
    [[nodiscard]] static ShapeBuildResult success(PartEntityPtr root_part) {
        ShapeBuildResult result;
        result.m_success = true;
        result.m_rootPart = std::move(root_part);
        return result;
    }

    /**
     * @brief Create a failure result
     * @param message Error description
     * @return Failure result
     */
    [[nodiscard]] static ShapeBuildResult failure(const std::string& message) {
        ShapeBuildResult result;
        result.m_success = false;
        result.m_errorMessage = message;
        return result;
    }

    /// Get total entity count
    [[nodiscard]] size_t totalEntityCount() const {
        return m_vertexCount + m_edgeCount + m_wireCount + m_faceCount + m_shellCount +
               m_solidCount + m_compoundCount + 1; // +1 for part
    }
};

/**
 * @brief Builder class for creating entity hierarchy from OCC shapes
 *
 * ShapeBuilder handles the complete process of:
 * 1. Traversing an OCC TopoDS_Shape hierarchy
 * 2. Creating corresponding GeometryEntity objects
 * 3. Establishing parent-child relationships
 * 4. Adding entities to a GeometryDocument
 * 5. Optionally generating render data and mesh metadata
 *
 * Usage:
 * @code
 * GeometryDocumentPtr doc = GeometryDocument::create();
 * ShapeBuilder builder(doc);
 *
 * TopoDS_Shape shape = ...; // From reader or construction
 * auto result = builder.buildFromShape(shape, "MyPart");
 *
 * if (result.m_success) {
 *     // Part and all sub-entities are now in the document
 *     auto part = result.m_rootPart;
 * }
 * @endcode
 */
class ShapeBuilder {
public:
    /**
     * @brief Construct a builder for the given document
     * @param document Target document to add entities to
     */
    explicit ShapeBuilder(GeometryDocumentPtr document);

    /**
     * @brief Destructor
     */
    ~ShapeBuilder() = default;

    // Non-copyable, non-movable
    ShapeBuilder(const ShapeBuilder&) = delete;
    ShapeBuilder& operator=(const ShapeBuilder&) = delete;
    ShapeBuilder(ShapeBuilder&&) = delete;
    ShapeBuilder& operator=(ShapeBuilder&&) = delete;

    /**
     * @brief Build entity hierarchy from a shape
     * @param shape The OCC shape to process
     * @param part_name Name for the created part entity
     * @param options Build options
     * @param progress_callback Optional progress callback
     * @return Build result with root part and statistics
     */
    [[nodiscard]] ShapeBuildResult
    buildFromShape(const TopoDS_Shape& shape,
                   const std::string& part_name = "Part",
                   const ShapeBuildOptions& options = ShapeBuildOptions(),
                   BuildProgressCallback progress_callback = nullptr);

    /**
     * @brief Get the target document
     * @return Shared pointer to the document
     */
    [[nodiscard]] GeometryDocumentPtr document() const { return m_document; }

private:
    /**
     * @brief Build sub-shape entities recursively
     * @param shape Shape to process
     * @param parent Parent entity to attach children to
     * @param options Build options
     * @param result Result to update with statistics
     * @param progress_callback Progress callback
     * @param progress_base Base progress value
     * @param progress_scale Progress scaling factor
     */
    void buildSubShapes(const TopoDS_Shape& shape,
                        const GeometryEntityPtr& parent,
                        const ShapeBuildOptions& options,
                        ShapeBuildResult& result,
                        BuildProgressCallback& progress_callback,
                        double progress_base,
                        double progress_scale);

    /**
     * @brief Create entity for a specific shape type
     * @param shape The OCC shape
     * @return Created entity, or nullptr if shape type not supported
     */
    [[nodiscard]] GeometryEntityPtr createEntityForShape(const TopoDS_Shape& shape);

    /**
     * @brief Generate render data for a part
     * @param part The part entity
     * @param options Build options (contains tessellation params)
     * @param part_index Index for color generation
     * @return Generated render data
     */
    [[nodiscard]] PartRenderDataPtr generateRenderData(const PartEntityPtr& part,
                                                       const ShapeBuildOptions& options,
                                                       size_t part_index);

    /**
     * @brief Generate mesh metadata for a part
     * @param part The part entity
     * @return Generated mesh metadata
     */
    [[nodiscard]] PartMeshMetadataPtr generateMeshMetadata(const PartEntityPtr& part);

    /**
     * @brief Tessellate a face for rendering
     * @param face The face entity
     * @param params Tessellation parameters
     * @param color Color for vertices
     * @return Render face data
     */
    [[nodiscard]] RenderFace tessellateFace(const TopoDS_Face& face,
                                            const TessellationParams& params,
                                            const RenderColor& color);

    /**
     * @brief Discretize an edge for rendering
     * @param edge The edge entity
     * @param params Tessellation parameters
     * @return Render edge data
     */
    [[nodiscard]] RenderEdge discretizeEdge(const TopoDS_Edge& edge,
                                            const TessellationParams& params);

private:
    GeometryDocumentPtr m_document; ///< Target document
    size_t m_partCounter{0};        ///< Counter for part index (color generation)
};

} // namespace OpenGeoLab::Geometry
