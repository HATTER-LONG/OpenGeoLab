/**
 * @file shape_builder.hpp
 * @brief Builder for creating entity hierarchies from OCC shapes
 *
 * ShapeBuilder traverses an OCC TopoDS_Shape and creates corresponding
 * GeometryEntity objects, establishing parent-child relationships to
 * form a complete entity hierarchy within a GeometryDocument.
 *
 * @note Uses TopTools_IndexedMapOfShape to ensure each unique topological
 * element is only created once, even when shared by multiple parent shapes.
 */

#pragma once

#include "entity/part_entity.hpp"
#include "geometry_documentImpl.hpp"
#include "util/progress_callback.hpp"

#include <TopTools_IndexedMapOfShape.hxx>
#include <kangaroo/util/noncopyable.hpp>
#include <unordered_map>

class TopoDS_Face;
class TopoDS_Shape;
class TopoDS_Edge;

namespace OpenGeoLab::Geometry {

/**
 * @brief Result of a shape build operation
 */
struct ShapeBuildResult {
    bool m_success{false};      ///< Whether the build succeeded
    std::string m_errorMessage; ///< Error message if failed
    PartEntityPtr m_rootPart;   ///< Root part entity created

    size_t m_vertexCount{0};   ///< Number of vertex entities created
    size_t m_edgeCount{0};     ///< Number of edge entities created
    size_t m_wireCount{0};     ///< Number of wire entities created
    size_t m_faceCount{0};     ///< Number of face entities created
    size_t m_shellCount{0};    ///< Number of shell entities created
    size_t m_solidCount{0};    ///< Number of solid entities created
    size_t m_compoundCount{0}; ///< Number of compound entities created

    /**
     * @brief Create a success result
     * @param root_part The created root part entity
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
     */
    [[nodiscard]] static ShapeBuildResult failure(const std::string& message) {
        ShapeBuildResult result;
        result.m_success = false;
        result.m_errorMessage = message;
        return result;
    }

    /**
     * @brief Get total count of all created entities
     * @return Sum of all entity type counts plus one for the part
     */
    [[nodiscard]] size_t totalEntityCount() const {
        return m_vertexCount + m_edgeCount + m_wireCount + m_faceCount + m_shellCount +
               m_solidCount + m_compoundCount + 1; // +1 for part
    }
};

/**
 * @brief Builder for creating entity hierarchies from OCC shapes
 *
 * Recursively traverses TopoDS_Shape structures and creates corresponding
 * GeometryEntity objects with proper parent-child relationships.
 *
 * Uses shape indexing to ensure each unique topological element (vertex, edge,
 * face, etc.) is created only once, even when shared by multiple parent shapes.
 */
class ShapeBuilder : public Kangaroo::Util::NonCopyMoveable {
public:
    /**
     * @brief Construct a shape builder for a document
     * @param document Target document for created entities
     */
    explicit ShapeBuilder(GeometryDocumentImplPtr document);
    ~ShapeBuilder() = default;

    /**
     * @brief Build entity hierarchy from an OCC shape
     * @param shape Source OCC shape
     * @param part_name Name for the root part entity
     * @param progress_callback Optional progress reporting callback
     * @return Build result with root part and entity counts
     *
     * @note Each unique topological element creates exactly one entity.
     * Shared elements (e.g., edges shared by two faces) are represented
     * by a single entity with multiple parent relationships.
     */
    [[nodiscard]] ShapeBuildResult
    buildFromShape(const TopoDS_Shape& shape,
                   const std::string& part_name = "Part",
                   Util::ProgressCallback progress_callback = Util::NO_PROGRESS_CALLBACK);

    /**
     * @brief Get the target document
     */
    [[nodiscard]] GeometryDocumentImplPtr document() const { return m_document; }

private:
    /**
     * @brief Map from shape index to created entity
     *
     * Shape index comes from TopTools_IndexedMapOfShape which assigns
     * a unique integer index to each unique shape in a model.
     */
    using ShapeEntityMap = std::unordered_map<int, GeometryEntityPtr>;

    /**
     * @brief Build all entities from indexed shapes
     * @param shape_map Indexed map of all unique shapes
     * @param shape_entity_map Output map from shape index to entity
     * @param result Build result to update counts
     */
    void buildEntitiesFromShapeMap(const TopTools_IndexedMapOfShape& shape_map,
                                   ShapeEntityMap& shape_entity_map,
                                   ShapeBuildResult& result);

    /**
     * @brief Build parent-child relationships based on topological structure
     * @param root_shape The root shape to traverse
     * @param shape_map Indexed map of all unique shapes
     * @param shape_entity_map Map from shape index to entity
     * @param root_entity The root entity (Part or container)
     */
    void buildRelationships(const TopoDS_Shape& root_shape,
                            const TopTools_IndexedMapOfShape& shape_map,
                            const ShapeEntityMap& shape_entity_map,
                            const GeometryEntityPtr& root_entity);

    /**
     * @brief Recursively build parent-child relationships
     * @param parent_shape Parent shape
     * @param parent_entity Parent entity
     * @param shape_map Indexed map of all unique shapes
     * @param shape_entity_map Map from shape index to entity
     */
    void buildChildRelationships(const TopoDS_Shape& parent_shape,
                                 const GeometryEntityPtr& parent_entity,
                                 const TopTools_IndexedMapOfShape& shape_map,
                                 const ShapeEntityMap& shape_entity_map);

    /**
     * @brief Create appropriate entity type for a shape
     */
    [[nodiscard]] GeometryEntityPtr createEntityForShape(const TopoDS_Shape& shape);

    /**
     * @brief Update entity counts in the result
     */
    void updateEntityCounts(const TopoDS_Shape& shape, ShapeBuildResult& result);

private:
    GeometryDocumentImplPtr m_document; ///< Target document
};

} // namespace OpenGeoLab::Geometry