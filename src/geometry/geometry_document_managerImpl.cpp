/**
 * @file geometry_document_managerImpl.cpp
 * @brief Implementation of GeometryDocumentManagerImpl
 */

#include "geometry_document_managerImpl.hpp"
#include "geometry_documentImpl.hpp"

namespace OpenGeoLab::Geometry {

std::shared_ptr<GeometryDocumentManagerImpl> GeometryDocumentManagerImpl::instance() {
    static auto instance = std::make_shared<GeometryDocumentManagerImpl>();
    return instance;
}

GeometryDocumentPtr GeometryDocumentManagerImpl::currentDocument() {
    if(!m_currentDocument) {
        return newDocument();
    }
    return m_currentDocument;
}

GeometryDocumentPtr GeometryDocumentManagerImpl::newDocument() { return newDocumentImplType(); }

GeometryDocumentImplPtr GeometryDocumentManagerImpl::currentDocumentImplType() {
    if(!m_currentDocument) {
        return newDocumentImplType();
    }
    return m_currentDocument;
}

GeometryDocumentImplPtr GeometryDocumentManagerImpl::newDocumentImplType() {
    m_currentDocument = std::make_shared<GeometryDocumentImpl>();
    return m_currentDocument;
}

GeometryDocumentManagerImplSingletonFactory::tObjectSharedPtr
GeometryDocumentManagerImplSingletonFactory::instance() const {
    return GeometryDocumentManagerImpl::instance();
}

} // namespace OpenGeoLab::Geometry