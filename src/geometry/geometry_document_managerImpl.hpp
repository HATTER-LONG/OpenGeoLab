#pragma once

#include "geometry/geometry_document_manager.hpp"
#include "geometry_documentImpl.hpp"

namespace OpenGeoLab::Geometry {
class GeometryDocumentManagerImpl : public GeometryDocumentManager {
public:
    static std::shared_ptr<GeometryDocumentManagerImpl> instance();

    GeometryDocumentManagerImpl() = default;
    ~GeometryDocumentManagerImpl() = default;

    [[nodiscard]] GeometryDocumentPtr currentDocument() override;

    [[nodiscard]] GeometryDocumentPtr newDocument() override;

    [[nodiscard]] GeometryDocumentImplPtr currentDocumentImplType();

    [[nodiscard]] GeometryDocumentImplPtr newDocumentImplType();

private:
    GeometryDocumentImplPtr m_currentDocument;
};

class GeometryDocumentManagerImplSingletonFactory : public IGeoDocumentManagerSingletonFactory {
public:
    GeometryDocumentManagerImplSingletonFactory() = default;
    ~GeometryDocumentManagerImplSingletonFactory() override = default;

    tObjectSharedPtr instance() const override;
};

} // namespace OpenGeoLab::Geometry