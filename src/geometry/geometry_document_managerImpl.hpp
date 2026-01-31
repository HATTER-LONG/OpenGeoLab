/**
 * @file geometry_document_managerImpl.hpp
 * @brief Implementation of GeometryDocumentManager for document lifecycle
 *
 * Provides the concrete implementation of document management including
 * singleton access and document creation.
 */

#pragma once

#include "geometry/geometry_document_manager.hpp"
#include "geometry_documentImpl.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Concrete implementation of GeometryDocumentManager
 *
 * Manages the lifecycle of geometry documents and provides singleton access.
 * Maintains a reference to the current active document.
 */
class GeometryDocumentManagerImpl : public GeometryDocumentManager {
public:
    /**
     * @brief Get the singleton instance of the document manager
     * @return Shared pointer to the document manager instance
     */
    static std::shared_ptr<GeometryDocumentManagerImpl> instance();

    GeometryDocumentManagerImpl() = default;
    ~GeometryDocumentManagerImpl() = default;

    [[nodiscard]] GeometryDocumentPtr currentDocument() override;

    [[nodiscard]] GeometryDocumentPtr newDocument() override;

    /**
     * @brief Get current document with implementation type
     * @return Shared pointer to current document implementation
     * @note Creates a new document if none exists
     */
    [[nodiscard]] GeometryDocumentImplPtr currentDocumentImplType();

    /**
     * @brief Create a new document with implementation type
     * @return Shared pointer to newly created document implementation
     */
    [[nodiscard]] GeometryDocumentImplPtr newDocumentImplType();

private:
    GeometryDocumentImplPtr m_currentDocument; ///< Current active document
};

/**
 * @brief Singleton factory for GeometryDocumentManagerImpl
 */
class GeometryDocumentManagerImplSingletonFactory : public IGeoDocumentManagerSingletonFactory {
public:
    GeometryDocumentManagerImplSingletonFactory() = default;
    ~GeometryDocumentManagerImplSingletonFactory() override = default;

    tObjectSharedPtr instance() const override;
};

} // namespace OpenGeoLab::Geometry