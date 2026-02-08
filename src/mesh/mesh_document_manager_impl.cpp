/**
 * @file mesh_document_manager_impl.cpp
 * @brief Implementation of MeshDocumentManagerImpl
 */

#include "mesh_document_manager_impl.hpp"

#include "util/logger.hpp"

namespace OpenGeoLab::Mesh {

std::shared_ptr<MeshDocumentManagerImpl> MeshDocumentManagerImpl::instance() {
    static auto s_instance = std::make_shared<MeshDocumentManagerImpl>();
    return s_instance;
}

MeshDocumentPtr MeshDocumentManagerImpl::currentDocument() {
    if(!m_currentDocument) {
        return newDocument();
    }
    return m_currentDocument;
}

MeshDocumentPtr MeshDocumentManagerImpl::newDocument() {
    if(!m_currentDocument) {
        m_currentDocument = std::make_shared<MeshDocumentImpl>();
    } else {
        m_currentDocument->clear();
    }
    LOG_TRACE("MeshDocumentManagerImpl: new/cleared mesh document");
    return m_currentDocument;
}

std::shared_ptr<MeshDocumentImpl> MeshDocumentManagerImpl::currentDocumentImpl() {
    if(!m_currentDocument) {
        (void)newDocument();
    }
    return m_currentDocument;
}

MeshDocumentManagerImplSingletonFactory::tObjectSharedPtr
MeshDocumentManagerImplSingletonFactory::instance() const {
    return MeshDocumentManagerImpl::instance();
}

} // namespace OpenGeoLab::Mesh
