/**
 * @file geometry_document_manager.hpp
 * @brief Singleton manager for geometry document lifecycle
 *
 * GeometryDocumentManager provides centralized access to geometry documents,
 * managing document creation and the current active document reference.
 */

#pragma once

#include "geometry/geometry_document.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Geometry {

/**
 * @brief Singleton manager for geometry document lifecycle
 *
 * Provides access to the current document and creation of new documents.
 * The manager ensures there is always an active document available.
 */
class GeometryDocumentManager : public Kangaroo::Util::NonCopyMoveable {
public:
    virtual ~GeometryDocumentManager() = default;

    /**
     * @brief Get the singleton instance
     * @return Reference to the global GeometryDocumentManager
     */
    static GeometryDocumentManager& instance();

    /**
     * @brief Get the currently active document
     * @return Shared pointer to the current GeometryDocument
     */
    [[nodiscard]] virtual GeometryDocumentPtr currentDocument() = 0;

    /**
     * @brief Create a new geometry document
     * @return Shared pointer to the newly created document
     * @note The new document becomes the current document
     */
    [[nodiscard]] virtual GeometryDocumentPtr newDocument() = 0;

protected:
    GeometryDocumentManager() = default;
};

/**
 * @brief Factory interface for creating GeometryDocumentManager singleton
 */
class IGeoDocumentManagerSingletonFactory
    : public Kangaroo::Util::FactoryTraits<IGeoDocumentManagerSingletonFactory,
                                           GeometryDocumentManager> {
public:
    IGeoDocumentManagerSingletonFactory() = default;
    virtual ~IGeoDocumentManagerSingletonFactory() = default;

    /**
     * @brief Get or create the singleton instance
     * @return Shared pointer to the GeometryDocumentManager instance
     */
    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace OpenGeoLab::Geometry
