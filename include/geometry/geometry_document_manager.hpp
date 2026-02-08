/**
 * @file geometry_document_manager.hpp
 * @brief Geometry document manager interface for document lifecycle management
 *
 * Provides abstract interface for managing geometry documents including
 * creation, access, and lifecycle operations.
 */

#pragma once
#include "geometry/geometry_document.hpp"
#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Geometry {

/**
 * @brief Abstract interface for managing geometry documents
 *
 * Provides access to the current document and creation of new documents.
 * This is the main entry point for document management operations.
 */
class GeometryDocumentManager : public Kangaroo::Util::NonCopyMoveable {
public:
    ~GeometryDocumentManager() = default;

    /**
     * @brief Get the current active document
     * @return Shared pointer to the current geometry document
     * @note Creates a new document if none exists
     */
    [[nodiscard]] virtual GeometryDocumentPtr currentDocument() = 0;

    /**
     * @brief Create a new empty document and make it current
     * @return Shared pointer to the newly created document
     */
    [[nodiscard]] virtual GeometryDocumentPtr newDocument() = 0;

protected:
    GeometryDocumentManager() = default;
};

/**
 * @brief Singleton factory interface for GeometryDocumentManager
 */
class IGeoDocumentManagerSingletonFactory
    : public Kangaroo::Util::FactoryTraits<IGeoDocumentManagerSingletonFactory,
                                           GeometryDocumentManager> {
public:
    IGeoDocumentManagerSingletonFactory() = default;
    virtual ~IGeoDocumentManagerSingletonFactory() = default;

    /**
     * @brief Get the singleton instance of the document manager
     * @return Shared pointer to the document manager instance
     */
    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace OpenGeoLab::Geometry

#define GeoDocumentMgrInstance                                                                     \
    g_ComponentFactory.getInstanceObject<Geometry::IGeoDocumentManagerSingletonFactory>()
