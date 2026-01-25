#pragma once

#include "geometry_document.hpp"

#include <kangaroo/util/noncopyable.hpp>

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

using DocumentId = uint64_t;

/**
 * @brief Singleton manager for all GeometryDocument instances.
 *
 * Responsibilities:
 * - Own documents and keep them alive
 * - Track / switch current document
 */
class GeometryManager : public Kangaroo::Util::NonCopyMoveable {
public:
    static GeometryManager& instance();

    DocumentId addDocument(const GeometryDocument::Ptr& document);
    bool removeDocument(DocumentId document_id);

    [[nodiscard]] GeometryDocument::Ptr document(DocumentId document_id) const;
    [[nodiscard]] std::vector<DocumentId> documentIds() const;

    [[nodiscard]] DocumentId currentDocumentId() const;
    [[nodiscard]] GeometryDocument::Ptr currentDocument() const;

    bool setCurrentDocument(DocumentId document_id);

    void clear();

private:
    GeometryManager() = default;
    ~GeometryManager() = default;

    mutable std::mutex m_mutex;
    std::unordered_map<DocumentId, GeometryDocument::Ptr> m_documents;

    DocumentId m_currentDocumentId{0};
    DocumentId m_nextDocumentId{1};
};

} // namespace OpenGeoLab::Geometry
