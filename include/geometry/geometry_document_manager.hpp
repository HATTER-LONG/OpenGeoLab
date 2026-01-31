/**
 * @file geometry_document_manager.hpp
 * @brief Singleton manager for geometry documents
 *
 * Provides centralized document management with singleton access
 * for creating and retrieving geometry documents.
 */

#pragma once
#include "geometry/geometry_document.hpp"
#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Geometry {

/**
 * @brief Singleton manager for geometry documents
 *
 * Manages the lifecycle of geometry documents in the application.
 * Currently supports a single active document; future versions may
 * support multiple documents.
 */
class GeometryDocumentManager : public Kangaroo::Util::NonCopyMoveable {
public:
    ~GeometryDocumentManager() = default;

    /**
     * @brief Get the singleton instance
     * @return Reference to the document manager
     */
    static GeometryDocumentManager& instance();

    /**
     * @brief Get the current active document
     * @return Shared pointer to current document, or nullptr if none
     */
    [[nodiscard]] virtual GeometryDocumentPtr currentDocument() = 0;

    /**
     * @brief Create a new empty document and set it as current
     * @return Shared pointer to the new document
     */
    [[nodiscard]] virtual GeometryDocumentPtr newDocument() = 0;

protected:
    GeometryDocumentManager() = default;
};

/**
 * @brief Factory interface for the document manager singleton
 */
class IGeoDocumentManagerSingletonFactory
    : public Kangaroo::Util::FactoryTraits<IGeoDocumentManagerSingletonFactory,
                                           GeometryDocumentManager> {
public:
    IGeoDocumentManagerSingletonFactory() = default;
    virtual ~IGeoDocumentManagerSingletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace OpenGeoLab::Geometry
