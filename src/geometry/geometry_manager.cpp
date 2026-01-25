#include "geometry/geometry_manager.hpp"

namespace OpenGeoLab::Geometry {

GeometryManager& GeometryManager::instance() {
    static GeometryManager singleton;
    return singleton;
}

DocumentId GeometryManager::addDocument(const GeometryDocument::Ptr& document) {
    if(!document) {
        return 0;
    }

    std::scoped_lock lock(m_mutex);

    const DocumentId id = m_nextDocumentId++;
    m_documents.emplace(id, document);

    // If no current document, set immediately.
    if(m_currentDocumentId == 0) {
        m_currentDocumentId = id;
    }

    return id;
}

bool GeometryManager::removeDocument(DocumentId document_id) {
    std::scoped_lock lock(m_mutex);

    const auto it = m_documents.find(document_id);
    if(it == m_documents.end()) {
        return false;
    }

    m_documents.erase(it);

    if(m_currentDocumentId == document_id) {
        m_currentDocumentId = 0;
        if(!m_documents.empty()) {
            m_currentDocumentId = m_documents.begin()->first;
        }
    }

    return true;
}

GeometryDocument::Ptr GeometryManager::document(DocumentId document_id) const {
    std::scoped_lock lock(m_mutex);

    const auto it = m_documents.find(document_id);
    if(it == m_documents.end()) {
        return nullptr;
    }

    return it->second;
}

std::vector<DocumentId> GeometryManager::documentIds() const {
    std::scoped_lock lock(m_mutex);

    std::vector<DocumentId> ids;
    ids.reserve(m_documents.size());

    for(const auto& [id, _] : m_documents) {
        ids.push_back(id);
    }

    return ids;
}

DocumentId GeometryManager::currentDocumentId() const {
    std::scoped_lock lock(m_mutex);
    return m_currentDocumentId;
}

GeometryDocument::Ptr GeometryManager::currentDocument() const {
    std::scoped_lock lock(m_mutex);

    const auto it = m_documents.find(m_currentDocumentId);
    if(it == m_documents.end()) {
        return nullptr;
    }
    return it->second;
}

bool GeometryManager::setCurrentDocument(DocumentId document_id) {
    std::scoped_lock lock(m_mutex);

    if(document_id == 0) {
        m_currentDocumentId = 0;
        return true;
    }

    if(m_documents.find(document_id) == m_documents.end()) {
        return false;
    }

    m_currentDocumentId = document_id;
    return true;
}

void GeometryManager::clear() {
    std::scoped_lock lock(m_mutex);
    m_documents.clear();
    m_currentDocumentId = 0;
    m_nextDocumentId = 1;
}

} // namespace OpenGeoLab::Geometry
