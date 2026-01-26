/**
 * @file geometry_builder.hpp
 * @brief Geometry entity hierarchy builder from OCC shapes
 *
 * GeometryBuilder traverses OpenCASCADE TopoDS_Shape topology and constructs
 * the corresponding GeometryEntity hierarchy with proper parent-child relationships.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "geometry/geometry_entity.hpp"
#include "geometry/part_entity.hpp"

#include <TopoDS_Shape.hxx>

#include <functional>
#include <string>

namespace OpenGeoLab::Geometry {

/**
 * @brief Progress callback for geometry building operations
 * @param progress Progress value in [0.0, 1.0]
 * @param message Human-readable status message
 * @return false to request cancellation
 */
using BuildProgressCallback = std::function<bool(double progress, const std::string& message)>;

/**
 * @brief Result structure for geometry building operations
 */
struct BuildResult {
    bool m_success{false};      ///< Whether the build operation succeeded
    std::string m_errorMessage; ///< Error message if failed
    PartEntityPtr m_partEntity; ///< The root part entity created

    /**
     * @brief Create a success result
     * @param part_entity The created part entity
     * @return Success BuildResult
     */
    [[nodiscard]] static BuildResult success(PartEntityPtr part_entity) {
        BuildResult result;
        result.m_success = true;
        result.m_partEntity = std::move(part_entity);
        return result;
    }

    /**
     * @brief Create a failure result
     * @param message Error description
     * @return Failure BuildResult
     */
    [[nodiscard]] static BuildResult failure(const std::string& message) {
        BuildResult result;
        result.m_success = false;
        result.m_errorMessage = message;
        return result;
    }
};

/**
 * @brief Builder for constructing geometry entity hierarchies from OCC shapes
 *
 * GeometryBuilder performs a recursive traversal of the OCC TopoDS_Shape topology
 * and creates corresponding GeometryEntity objects with proper parent-child
 * relationships. Entities are deduplicated using the shape index in GeometryDocument.
 *
 * Build process:
 * 1. Create a PartEntity as the root container
 * 2. Recursively traverse the shape topology
 * 3. Create entities for each unique sub-shape
 * 4. Establish parent-child relationships
 * 5. Register all entities with the GeometryDocument
 *
 * @note Shape deduplication is based on OCC's TopoDS_Shape::IsSame() semantics.
 */
class GeometryBuilder {
public:
    /**
     * @brief Construct a builder for a specific document
     * @param document Target document for entity registration
     */
    explicit GeometryBuilder(GeometryDocumentPtr document);

    ~GeometryBuilder() = default;

    /**
     * @brief Build a complete entity hierarchy from an OCC shape
     * @param shape Source shape to build from
     * @param part_name Optional name for the created part
     * @param progress_callback Optional progress reporting callback
     * @return BuildResult with the created PartEntity or error information
     *
     * @note The shape should be valid and non-null.
     * @note Progress callback returning false will abort the operation.
     */
    [[nodiscard]] BuildResult buildFromShape(const TopoDS_Shape& shape,
                                             const std::string& part_name = "",
                                             BuildProgressCallback progress_callback = nullptr);

private:
    /**
     * @brief Recursively create entities for a shape and its children
     * @param shape Shape to process
     * @param parent_entity Parent entity for relationship (may be null for root)
     * @return Created or existing entity for the shape
     */
    [[nodiscard]] GeometryEntityPtr createEntityForShape(const TopoDS_Shape& shape,
                                                         const GeometryEntityPtr& parent_entity);

    /**
     * @brief Create a typed entity based on shape type
     * @param shape Shape to wrap
     * @return New entity instance (not yet registered)
     */
    [[nodiscard]] GeometryEntityPtr createTypedEntity(const TopoDS_Shape& shape);

    /**
     * @brief Count total sub-shapes for progress calculation
     * @param shape Root shape
     * @return Total count of all sub-shapes
     */
    [[nodiscard]] size_t countSubShapes(const TopoDS_Shape& shape) const;

    /**
     * @brief Report build progress
     * @param message Status message
     * @return false if cancelled
     */
    bool reportProgress(const std::string& message);

private:
    GeometryDocumentPtr m_document;           ///< Target document
    BuildProgressCallback m_progressCallback; ///< Progress callback
    size_t m_totalShapes{0};                  ///< Total shapes for progress
    size_t m_processedShapes{0};              ///< Processed shapes counter
};

} // namespace OpenGeoLab::Geometry
